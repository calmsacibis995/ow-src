/*
 * simple.c - Simple Classing Engine Example that types a file and
 * determines it's icon.
 *
 * cc -g -o simple1 -I$OPENWINHOME/include -L$OPENWINHOME/lib simple1.c -lce -ldl
 */

#include <stdio.h>
#include <desktop/ce.h>
#include <desktop/ce_err.h>

/* variable definitions */
CE_NAMESPACE 	f_name_space, t_name_space;
CE_ENTRY 		ftype_ent, ttype_ent;
CE_ATTRIBUTE 	fns_type, tns_icon, fns_attr, tns_attr;
int 			argcount;

main(int argc, char **argv)
{
	int  fd;
	char filename[81];
	char buf[256];
	int  bufsize, status;

	/*
	 * Initialize the Classing Engine
	 */
	status = ce_begin (NULL);
	if (status)	{
		fprintf (stderr, "Error initializing Classing Engine Database - Error no: %d.\n", status);
		exit (0);
	}
	
	/* Read in Namespace Entries */
	f_name_space = ce_get_namespace_id ("Files");
	if (!f_name_space)	{
		fprintf (stderr, "Cannot find File Namespace\n");
		ce_end();
		exit(0);
	}
	
	t_name_space = ce_get_namespace_id ("Types");
	if (!t_name_space)	{
		fprintf (stderr, "Cannot find Types namespace\n");
		ce_end();
		exit(0);
	}
	
	/*
	 * Get the attribute ID's that we're interested in
	 */
	fns_attr = ce_get_attribute_id (f_name_space, "FNS_TYPE");
	
	if (!fns_attr)	{
		fprintf (stderr, "Cannot find FNS_ATTR in Files\n");
		ce_end();
		exit(0);
	}

	tns_attr = ce_get_attribute_id (t_name_space, "TYPE_ICON");
	
	if (!tns_attr)	{
		fprintf (stderr, "Cannot find TYPE_ICON in Types\n");
		ce_end();
		exit(0);
	}
	
	/*  
	 * Start loop to read in filenames and type them
	 */
	while(1) {
		fprintf(stdout, "Filename: ");
		gets(filename);
		if ((strcmp(filename, "quit")) == 0)
			break;

		if ((fd = open (filename, 0)) == -1) {
			fprintf(stderr, "Cannot open: %s\n", filename);
			continue;
		}

		bufsize = read (fd, buf, sizeof (buf));
		if (bufsize <= 0) {
			fprintf(stderr, "Empty file or Directory: %s\n", filename);
			close (fd);
			continue;
		}

		/* Get a matching entry in the files namespace */
		argcount = 3;
		ftype_ent =  ce_get_entry (f_name_space, argcount, filename,
					   buf, bufsize);
		if (!ftype_ent)	{
			fprintf (stderr, "No match in Files Namespace\n");
			continue;
		}
	
		fns_type = ce_get_attribute (f_name_space, ftype_ent, fns_attr);
	
		if (!fns_type)	{
			fprintf (stderr, "No FNS_TYPE for entry in Files Namespace\n");
			continue;
		}
		else	{
			fprintf (stdout, "FNS_TYPE = %s\n", fns_type);
		
			/* Get a matching entry in the types namespace found from
			 * getting type from the files namespace and find icon
			 */
			argcount = 1;
			ttype_ent = ce_get_entry (t_name_space, argcount, fns_type);
			
			if (!ttype_ent)	{
				fprintf (stderr, "No match in Types namespace\n");
				continue;
			}
			tns_icon = ce_get_attribute (t_name_space, ttype_ent,
						     tns_attr);
		
			if (!tns_icon)	{
				fprintf (stderr, "No TYPE_ICON in Types namespace\n");
				continue;
			}
			else
				fprintf (stdout, "TYPE_ICON = %s\n", tns_icon);
		}
	}
	
	ce_end();
	exit(0);
}
