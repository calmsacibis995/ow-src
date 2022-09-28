#ifndef lint
static char sccsid[] = "@(#)ce_db_merge.c	3.5 03/16/94 Copyright 1989 Sun Microsystems, Inc.";
#endif lint

/* @(#)ce_db_merge.c	2.2 - 91/01/23 */


/* ce_db_merge.c */

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
int database_type=0;

char *progname;

main (argc, argv)
int	argc;
char	*argv[];

{
	extern void print_error ();
	extern	int merge_db ();
	extern	char	*get_default_db_path ();
	int	print_db();
	int	doing_from_ascii = 0;
	int	doing_to_ascii = 0;
	char	*filename;
	char	*dbfile = NULL;
	char	*db_name = NULL;
	int	db_type;
	
	int status;
	(void)setlocale (LC_MESSAGES, "");

	(void)textdomain("SUNW_DESKSET_CE");

	progname = (char *) strdup (argv[0]);
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
	database_type = db_type;
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
	if ((status = merge_db(db_name, db_type, filename, dbfile)) != 0) {
		if (!dbfile)
			dbfile = get_default_db_path (db_type);
		
		print_error(status, db_name, filename, dbfile);
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
			 "user | system | network -from_ascii file-name [ -db_file filename ]");
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
	

