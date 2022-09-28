#ifndef lint
static char sccsid[] = "@(#)ce_db_build.c	3.4 01/22/93 Copyright 1989 Sun Microsystems, Inc.";
#endif lint

#include	<sys/param.h>
#include	<stdio.h>
#include	<locale.h>
#include	"ce_int.h"
static char lcurly[] = "{";
static char rcurly[] = "}";
static char lparen[] = "(";
static char rparen[] = ")";

static char NS_NAME[] = "NS_NAME";
static char NS_ATTR[] = "NS_ATTR";
static char NS_MANAGER[] = "NS_MANAGER";
static char NS_ENTRIES[] = "NS_ENTRIES";
static char l_than[] = "<";
static char g_than[] = ">";
static char equals[] = "=";
static char newline[] = "\n"; 
static char comma[] = ",";

char *progname;

main (argc, argv)
int	argc;
char	*argv[];

{
	extern void print_error ();
	extern	int build_db ();
	extern	char	*get_default_db_path();
	int	print_db();
	int	db_type;
	int	doing_from_ascii = 0;
	int	doing_to_ascii = 0;
	char	*filename;
	char	*dbfile = NULL;
	char	*db_name = NULL;
	
	int status;
	(void)setlocale (LC_MESSAGES, ""); 
	
	(void)textdomain("SUNW_DESKSET_CE");

	progname = argv[0];
	
	if (argc < 4)	{
		print_error (CE_ERR_WRONG_ARGUMENTS, NULL, NULL, NULL);
		exit (1);
	}


	
	/* look for first argument */
	if (strcmp (argv[1], user_db_name) == 0) {
		db_type = USER_DB;
	} else if (strcmp (argv[1], system_db_name) == 0) {
		db_type = SYSTEM_DB;
	} else if (strcmp (argv[1], network_db_name) == 0) {
		db_type = NETWORK_DB;
	} else {
		print_error (CE_ERR_WRONG_ARGUMENTS, NULL, NULL, NULL);
		exit (1);
	}

	db_name = argv [1];
	
	/* now parse the rest of the arguments */
	for (argc -= 2, argv += 2; argc > 0; argv++, argc--) {

		if (strcmp(*argv, "-from_ascii") == 0) {
			doing_from_ascii = 1;
			if (doing_to_ascii || argc < 2) {
				print_error(CE_ERR_WRONG_ARGUMENTS, NULL, NULL, NULL);
				exit(1);
			}

			filename = *++argv;
			argc--;

		} else if (strcmp(*argv, "-to_ascii") == 0) {
			doing_to_ascii = 1;
			if (doing_from_ascii || argc < 2) {
				print_error(CE_ERR_WRONG_ARGUMENTS, NULL, NULL, NULL);
				exit(1);
			}

			filename = *++argv;
			argc--;

		} else if (strcmp(*argv, "-db_file") == 0) {
			if (argc < 2) {
				print_error(CE_ERR_WRONG_ARGUMENTS, NULL, NULL, NULL);
				exit(1);
			}

			dbfile = *++argv;
			argc--;

			status = perform_db_op(NULL, db_type, SET_DB_FILENAME,
				(int*) dbfile);

			if (status) {
				print_error(status, db_name, filename, dbfile);
				exit(1);
			}

		} else {
			print_error(CE_ERR_WRONG_ARGUMENTS, NULL, NULL, NULL);
			exit(1);
		}
	}
	
	/* look for second and third argument */
	if (doing_from_ascii) {
		if ((status = build_db(db_name, db_type, filename)) != 0) {
			if (!dbfile)
				dbfile = get_default_db_path (db_type);
			
			print_error(status, db_name, filename, dbfile);
			exit(1);
		}
	} else if (doing_to_ascii) {
		if ((status = to_ascii(db_type, filename)) != 0) {
			if (!dbfile)
				dbfile = get_default_db_path (db_type);
			
			print_error(status, db_name, filename, dbfile);
			exit(1);
		}
	} else {
		print_error(CE_ERR_WRONG_ARGUMENTS, NULL, NULL, NULL);
		exit(1);
	}

	exit(0);

}

void
print_error (error_code, db_name, file_name, dbfile)
int	error_code;
char	*db_name;
char	*file_name;
char	*dbfile;

{
	switch (error_code) 	{
	case CE_ERR_WRONG_ARGUMENTS:
		fprintf (stderr, "%s %s %s\n", gettext ("Usage:"), progname,
			 "user | system | network [ -from_ascii | -to_ascii ] file-name [ -db_file filename ]");
		break;
	default:
		fprintf (stderr, "%s: %s\n", progname, 
			 gettext ("Cannot open or write database."));
		fprintf (stderr, "%s %s, %s %s, %s %s\n", gettext("Database type:"),
			 db_name, gettext("From ASCII file:"), file_name, 
			 gettext("Database file name:"), dbfile);
		break;
	}
	

}
	

int
print_db (db_type)
int db_type;


{
	CE_DB	ce_db;
	CE_NS	*curr_ns;
	CE_ENT	*curr_entry;
	CE_AV	*curr_av;
	CE_ATTR	*curr_attr;
	char *full_path;
	char *temp;
	int size;
	char db_path[MAXPATHLEN];
	int sts;
	int dd = 0;
	
	memset ((char *)&ce_db, 0, sizeof ce_db);
	
	
	if ((sts = perform_db_op (NULL, db_type, OPEN_DB_FOR_READ, &dd)) != 0)
		return (sts);
	if ((sts = perform_db_op (&ce_db, db_type, READ_DB, &dd)) != 0) {
		perform_db_op (NULL, NULL, CLOSE_DB, &dd);
		return (sts);
	}
	perform_db_op (NULL, NULL, CLOSE_DB, &dd);
	
	
	for (curr_ns = ce_db.namespaces; curr_ns != NULL;
	     curr_ns = curr_ns->next_ns)	{
		printf ("NS name: %s\n", curr_ns->ns_name);
		
		/* print out the attr defs */
		for (curr_attr = curr_ns->attr_def; curr_attr != NULL;
		     curr_attr = curr_attr->next_attr)	{
			printf ("  Attr info: Name %s Id %d\n",	
				curr_attr->a_name,
				curr_attr->a_id);
		}
		/* print out the entries */
		for (curr_entry = curr_ns->entries; curr_entry != NULL;
		     curr_entry = curr_entry->next_entry)	{
			printf ("  Entry info: Id %d\n", curr_entry->ent_id);
			printf ("    AVs\n");
			
			/* print out the avs */
			for (curr_av = curr_entry->avs; curr_av != NULL;
			     curr_av = curr_av->next_av) {
				printf ("    AV info: Id %d Size %d Val %s Type %s\n",	
					curr_av->a_id,
					curr_av->a_size,
					curr_av->a_val,
					curr_av->a_type);
			}	
		}
	}
}
int
print_ns (name, f)
char	*name;
FILE	*f;
{
	fprintf (f, "%s%s%s%s", NS_NAME, equals, name, newline);
}
int
print_av (av, f)
CE_AV	*av;
FILE	*f;
{
	/* we have an oddity here.  Some kinds of attribute value will
           contain characters that will bollix up hte parser when it
           tries to reread the written ascii file.  Currently the list
           is only two characters, the l_than and g_than characters.

           We will test for these, and use the "length" modifier on 
           the attribute value to override the parsing value of these
           characters. */

	if (strstr(av->a_val, l_than) || strstr(av->a_val, g_than))
		fprintf (f, "%s%s%s%s%s%d%s%s%s%s%s",
		 	lparen, av->a_id, comma, av->a_type, comma, strlen(av->a_val), l_than, 
		 	av->a_val, g_than, rparen, newline);
	else
		fprintf (f, "%s%s%s%s%s%s%s%s%s%s",
		 	lparen, av->a_id, comma, av->a_type, comma, l_than, 
		 	av->a_val, g_than, rparen, newline);
}
	
int
print_entry (entry, f)
CE_ENTRY entry;
FILE	*f;
{
	CE_AV	*curr_av;
	
	if (is_ns_entry (entry->flags))
		fprintf (f, "%s", lparen);
	else
		fprintf (f, "	%s%s", lparen, newline);
	
	for (curr_av = entry->avs; curr_av != NULL; curr_av = curr_av->next_av)
		print_av (curr_av, f);
	if (entry->next_entry == NULL)
		fprintf (f, "%s%s", rparen, newline);
	else
		fprintf (f, "%s", rparen);
}


int
to_ascii (db_type, ascii_path)
int	db_type;
char	*ascii_path;
{
	CE_DB	ce_db;
	CE_NS	*curr_ns;
	CE_ENT	*curr_entry;
	CE_AV	*curr_av;
	CE_ATTR	*curr_attr;
	char *full_path;
	char *temp;
	int size;
	char db_path[MAXPATHLEN];
	int sts;
	FILE *f = fopen (ascii_path, "w");
	
	int dd = 0;

	
	if (!f)
		return (CE_ERR_ERROR_OPENING_FILE);
	
	memset ((char *)&ce_db, 0, sizeof ce_db);
	
	
	if ((sts = perform_db_op (NULL, db_type, OPEN_DB_FOR_READ, &dd)) != 0)
		return (sts);
	if ((sts = perform_db_op (&ce_db, db_type, READ_DB, &dd)) != 0) {
		perform_db_op (NULL, NULL, CLOSE_DB, &dd);
		return (sts);
	}
	perform_db_op (NULL, NULL, CLOSE_DB, &dd);
	
	for (curr_ns = ce_db.namespaces; curr_ns != NULL;
	     curr_ns = curr_ns->next_ns)	{
		fprintf (f, "%s%s", lcurly, newline);
		print_ns (curr_ns->ns_name, f);
		
		curr_entry = curr_ns->entries;

		/* print out the ns entry, if there is one */
		fprintf (f, "%s%s", NS_ATTR, equals);
		if ((curr_entry &&
		     is_ns_entry (curr_entry->flags)))	{
			print_entry (curr_entry, f);
		}
		fprintf (f, "%s", newline);					

		/* print out the all the other entries */
		fprintf (f, "%s%s%s", NS_ENTRIES, equals, lparen);
		
		for (curr_entry = curr_ns->entries; curr_entry != NULL;
		     curr_entry = curr_entry->next_entry)	
			if (!is_ns_entry (curr_entry->flags))	{
				print_entry (curr_entry, f);
				
			}
		fprintf (f, "%s%s", rparen, newline);
		fprintf (f, "%s%s", rcurly, newline);
	}

	fclose (f);
	return (0);
	
	
}



					
	    

		
 
       
