#ifndef lint
static char sccsid[] = "@(#)ce_db_rw.c	3.2 10/28/92 Copyright 1989 Sun Microsystems, Inc.";
#endif lint

/* @(#)ce_db_rw.c	1.6 1/29/91
 *
 * Routines that read from/write to a CE database file in stored in XDR form
 */

#include	<stdio.h>
#include	<string.h>
#include	<malloc.h>

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<rpc/rpc.h>

#include	"ce_int.h"


/*	Test Version 1:  Read these in with fopen + XDRSTDIO.
 *	(Alternates include mmap + XDRMEM.)
 *
 *	It's OK to read and write empty CE database files
 */

int
read_ce_db (db, dd)
CE_DB 	*db;
int	dd;

{
	FILE	*f = fdopen (dd, "r");
	XDR 	x;
	char 	magic_buf [ sizeof ce_database_magic  ];
	char	version_buf [ sizeof ce_database_version  ];

	if (!f)
		return (CE_ERR_ERROR_READING_DB);

	if (fgets (magic_buf, sizeof magic_buf, f) == 0
	    || strcmp (magic_buf, ce_database_magic) != 0)	{
		(void)fclose(f);
		return (CE_ERR_BAD_DATABASE_FILE);
	}
	if (fgets (version_buf, sizeof version_buf, f) == 0
	    || strcmp (version_buf, ce_database_version) !=0)	{
		(void)fclose(f);
		return (CE_ERR_WRONG_DATABASE_VERSION);
	}
	    
	xdrstdio_create (&x, f, XDR_DECODE);
	if (xdr_ce_db_p (&x, &db) != TRUE) {
		xdr_free (xdr_CE_DB, (char *) db); 
		(void)fclose(f);
		
		return (CE_ERR_ERROR_READING_DB);
	}
	
	(void)fclose (f);
	
	return (0);
}

int
write_ce_db (db, dd)
CE_DB 	*db;
int	dd;

{
	FILE 	*f;
	XDR 	x;
	struct 	stat statb;
	long 	vers;
	int 	sts;

	if ((f = fdopen (dd, "w")) == 0)	
		return (CE_ERR_ERROR_READING_DB);
		
	sts = fprintf (f, ce_database_magic);
	sts = fprintf (f, ce_database_version);
	/* XXX - updates the file, which is a no-no sts = fflush (f); */
	
	xdrstdio_create (&x, f, XDR_ENCODE);
	if (xdr_ce_db_p (&x, &db) != TRUE) {
		(void)fclose(f);
		
		return (CE_ERR_ERROR_WRITING_DB);
	}
	
	(void)fclose (f);
	
	return (0);
}

void
free_av	(av)
CE_AV 	*av;
{
	free (av->a_id);
	free (av->a_type);
	free (av->a_val);
	free (av);
}

void
free_entry (entry)
CE_ENT	*entry;
{
	CE_AV	*curr_av, *next_av;
	
	/* XXX free (entry->match_vals.match_vals_val); */
	
	for (curr_av = entry->avs; curr_av != NULL; curr_av = next_av) {
		next_av = curr_av->next_av;
		free_av (curr_av);
	}
	free (entry);
}

void
free_attr (attr)
CE_ATTR	*attr;
{
	free (attr->a_name);
	free (attr);
}

void
free_name_space (ns)
CE_NS	*ns;
{
	CE_ENT	*curr_entry, *next_entry;
	CE_ATTR	*curr_attr, *next_attr;
	
	free (ns->ns_name);
	free (ns->ns_mgr_ptr);
	
	for (curr_entry = ns->entries; curr_entry != NULL; curr_entry =
	     next_entry)	{
		next_entry = curr_entry->next_entry;
		free_entry (curr_entry);
	}
	for (curr_attr = ns->attr_def; curr_attr != NULL; curr_attr =
	     next_attr)	{
		next_attr = curr_attr->next_attr;
		free_attr (curr_attr);
	}
	free (ns);
	
}


void
free_ce_db (db)
CE_DB *db;

{
	CE_NS	*curr_ns, *next_ns;
	
	for (curr_ns = db->namespaces; curr_ns != NULL; curr_ns = next_ns) {
		next_ns = curr_ns->next_ns;
		
		free_name_space (curr_ns);
	}
	/* XXX free (db->db_name); */
	free (db->db_path);
		
	free (db);
}
