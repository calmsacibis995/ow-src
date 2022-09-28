#ifndef lint
static char sccsid[] = "@(#)merge_ascii.c	3.4 03/16/94 Copyright 1989 Sun Microsystems, Inc.";
#endif lint

/* @(#)merge_ascii.c    2.2 - 91/01/23 */

#include <stdio.h>
#include <sys/param.h>
#include "ce_int.h"
#include "y.tab.h"

/* file used by input routine in lex */
FILE *infile; 

static parse_errors;


#define DEBUG
#ifdef DEBUG
#define DP(a)	printf a
#else
#define	DP(a)	
#endif

static CE_NAMESPACE current_namespace;
static CE_ENTRY	current_entry;
static CE_ENTRY	current_ns_entry;
static int	ns_entry_added;



extern int yyparse ();
extern char yytext [];
extern int yyleng;
extern char *progname;
extern CE_ENTRY ce_get_ns_entry();


/* reads in the database using yacc & lex */
int
merge_db (db_name, db_type, ascii_path, db_path)
char	*db_name;
int	db_type;
char	*ascii_path;
char	*db_path;

{

	int sts;
	int dd = 0;
	
	if ((infile = (FILE *) fopen (ascii_path, "r")) == NULL)
		return (-1);

	/* open the database */
	if ((sts = ce_begin(0)) != 0) return(sts);

	if ((sts = ce_start_write(db_name)) != 0) {
		DP(("ce_start_write(%s) failed (%d)\n", db_name, sts));
		return (sts);
	}

	yyparse ();

	if (parse_errors) {
		return(-1);
	} else {
		/* delete the current entry, which is empty now */
		ce_remove_entry(current_namespace, current_entry);

		if ((sts = ce_commit_write(0)) != 0) {
			DP(("ce_commit_write(0) failed (%d)\n", sts));
			return(sts);
		}
	}

	return(0);
}


void
yyerror (s)
char *s;

{
	extern int yylineno;
	printf ("YYERROR:%s, line %d, symbol %s\n", s, yylineno, yytext);

	parse_errors++;
}

caddr_t 
add_name_space(name, database_type)
char *name;
int database_type;
{
	int status;

	if ((current_namespace = ce_get_namespace_id_from_db (database_type, name)) == NULL)	{
		DP(("add_name_space(%s)\n", (char *)name));

		status = ce_add_namespace(name, &current_namespace);
		if (status) {
			fprintf(stderr, "%s: cannot add new namespace %s\n",
				progname, name);
			exit(1);
		}
	}
	

	DP(("add_name_space: new namespace %s\n", name));

	if ((current_ns_entry = ce_get_ns_entry(current_namespace)) == NULL) {
		status = ce_alloc_ns_entry(current_namespace,
			&current_ns_entry);
		if (status) {
			fprintf(stderr,
			"%s: could not allocate a new namespace entry: %d\n",
				progname, status);
			exit(1);
		}
	}

	status = ce_alloc_entry(current_namespace, &current_entry);
	if (status) {
		fprintf(stderr, "%s: could not allocate a new entry: %d\n",
			progname, status);
		exit(1);
	}



	ns_entry_added = 0;
}



/* Add an av to an entry */
caddr_t 
add_avlist (name, type, value)
caddr_t name;
caddr_t type;
caddr_t value;
{
	int *attr_a_id;
	int len;
	int status;

	/* horrible hack: we hide the length at the start of the value */

	len = *(int *) value;
	value += 4;

	DP(("add_avlist(%s, %s, <%s>)\n", name, type, value));

	if (! ns_entry_added) {
		
		status = ce_add_attribute(current_namespace, &current_ns_entry,
			name, type, value, len);

		if (status) {
			if (status == CE_ERR_ATTRIBUTE_EXISTS)	{
				status = ce_modify_attribute (current_namespace,
						     &current_ns_entry,
						     name,
						     type,
						     value,
						     len);
				if (status)	{
					fprintf(stderr,
						"%s: could not add entry (%s,%s,<%s>): %d\n",
						progname, name, type, value, status);
					exit (1);
				}
			}
			else	{
				
				fprintf(stderr,
					"%s: could not add entry (%s,%s,<%s>): %d\n",
					progname, name, type, value, status);
				exit (1);
			}
			
		}
	}
	else	{
		
		status = ce_add_attribute(current_namespace, &current_entry,
					  name, type, value, len);

		if (status) {
			if (status == CE_ERR_ATTRIBUTE_EXISTS)	{
				status = ce_modify_attribute (current_namespace,
						     &current_entry,
						     name,
						     type,
						     value,
						     len);

				if (status) {
					fprintf(stderr,
						"%s: could not add entry (%s,%s,<%s>): %d\n",
						progname, name, type, value, status);
					exit (1);
				}
			}
			else	{

				fprintf(stderr,
					"%s: could not add entry (%s,%s,<%s>): %d\n",
					progname, name, type, value, status);
				exit (1);
			}
		}
	}
	


}




/* allocates a ptr that points to the contents of yytext for an id
 */
caddr_t 
alloc_id ()
{
	char *loc_ptr;

	loc_ptr = (char *)strdup ((char *)yytext);

	if (! loc_ptr) {
		no_memory();
	}

	return (caddr_t) loc_ptr;
}


/* allocates a ptr that points to the contents of yytext for an attribute value
 */
caddr_t
alloc_val ()
{
	char *loc_ptr;
	int len;

	len = yyleng;

	/* subtract 2 because we're not going to store the value delimiters */
	len = len - 2;

	/* malloc memory to store value */
	loc_ptr = (char *) malloc (len + 5);

	/* horrible hack: hide the length in the first four bytes.  I'm too
	* lazy to do this right.
	*/
	*(int *) loc_ptr = len;

	/* copy the bytes over */
	memcpy(loc_ptr + 4, yytext + 1, len);
	loc_ptr [len + 4] = '\0';

	return (caddr_t) loc_ptr;
}


set_new_entry(is_ns)
{
	int status;

	DP(("set_new_entry(is_ns %d)\n", is_ns));


	if (is_ns) {
		ns_entry_added = 1;

		status = ce_add_entry(current_namespace, current_ns_entry);
		if (status) {
			fprintf(stderr,
			"%s: could not start a new namespace entry: %d\n",
				progname, status);
			exit(1);
		}

	} else {

		status = ce_add_entry(current_namespace, current_entry);
		if (status) {
			fprintf(stderr,
				"%s: could not start a new entry: %d\n",
				progname, status);
			exit(1);
		}

		status = ce_alloc_entry(current_namespace, &current_entry);
		if (status) {
			fprintf(stderr,
				"%s: could not allocate a new entry: %d\n",
				progname, status);
			exit(1);
		}
	}
}


no_memory()
{
	fprintf(stderr, "%s: out of memory! aborting...\n", progname);
	exit(1);
}

