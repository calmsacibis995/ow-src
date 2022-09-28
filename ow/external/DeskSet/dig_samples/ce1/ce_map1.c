/*
 * map1.c - Classing Engine example that print all the types in the Files 
 *         and Types namespaces.
 *
 * cc -g -o map1 -I$OPENWINHOME/include -L$OPENWINHOME/lib map1.c -lce -ldl
 */

#include <stdio.h>
#include <desktop/ce.h>
#include <desktop/ce_err.h>

void *type_map_func(CE_ATTRIBUTE tattr_handle, char *tattr_value, void *args);

/* variable definitions */
CE_NAMESPACE 	f_name_space, t_name_space;
CE_ENTRY		ttype_ent;
CE_ATTRIBUTE 	fns_attr, fns_type;

main(int argc, char **argv)
{
	int  status;
	void *map_func(), *type_map_func();

	/*
	 * Initialize the Classing Engine.
	 */ 
	status = ce_begin( NULL );
	if ( status ) {
		fprintf( stderr, "Error Initializing Classing Engine Database - Error no: %d.\n", status );
		exit( 0 );
	}

	/*
	 * Get Files and Types Entries.
	 */
	f_name_space = ce_get_namespace_id( "Files" );
	if ( !f_name_space ) {
		fprintf( stderr, "Cannot find File Namespace\n" );
		exit( 0 );
	}

	t_name_space = ce_get_namespace_id( "Types" );
	if ( !t_name_space ) {
		fprintf( stderr, "Cannot find Type Namespace\n" );
		exit( 0 );
	}

	/* Get the FNS_TYPE attribute ID */
	fns_attr = ce_get_attribute_id (f_name_space, "FNS_TYPE");

	if (!fns_attr)	{
		fprintf (stderr, "No FNS_TYPE in Files Namespace\n");
		ce_end();
		exit (0);
	}

	/* Call ce_map_through_entries() to pass each entry handle and
	 * namespace handle to the map_func()
	 */
	ce_map_through_entries (f_name_space, map_func, NULL);
	ce_end ();
	exit(0);

}

/*
 * Function to handle each entry as it is passed from the mapping
 * function
 */
void 
*map_func (CE_NAMESPACE fns_handle, CE_ENTRY ent_handle)
{
	int argcount = 1;
	
	/* Get File type value (FNS_TYPE) and print out */
	fns_type = ce_get_attribute (f_name_space, ent_handle, fns_attr);
	
	if (!fns_type)
		return (NULL);
	else	
		fprintf (stdout, "FNS_TYPE = %s\n", fns_type);
	
	/* Get matching entry in the Type namespace */
	ttype_ent = ce_get_entry (t_name_space, argcount, fns_type);
	if (!ttype_ent)	{
		fprintf (stderr, "No match in Type namespace\n");
		return (NULL);
	}
	
	/* Map through all the attributes of the entry and send to
	 * type_map_func()
	 */
	ce_map_through_attrs (t_name_space, ttype_ent, type_map_func, NULL);
	fprintf (stdout, "\n");
	return (NULL);
}



/*
 * Function to print all the Type attributes associated with the File
 * type
 *
 */
void 
*type_map_func (CE_ATTRIBUTE tattr_handle, char	*tattr_value, void *args)
{
	char	*attr_name;
	
	attr_name = ce_get_attribute_name (tattr_handle);
	
	if (attr_name)
		fprintf (stdout, "%s = %s\n", attr_name, tattr_value);
	
	return (NULL);
}
