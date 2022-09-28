/*
 *	(c) Copyright 1993 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */


/* NOTE: hacked up from xview defaults.h */

#ifndef	AUDIOTOOL_XV_DEFAULTS_H
#define	AUDIOTOOL_XV_DEFAULTS_H

#ident	"@(#)xv_defaults.h	1.2	92/12/14 SMI"

/*
 ***********************************************************************
 *			Definitions and Macros
 ***********************************************************************
 */

#define XV_DEFAULTS_MAX_VALUE_SIZE 	2048	/* move this to defaults.h */

#ifndef Bool
#define Bool int
#endif

#include <xview/xv_c_types.h>

/*
 ***********************************************************************
 *		Typedefs, enumerations, and structs
 ***********************************************************************
 */

typedef struct _default_pairs {
	char	*name;					/* Name of pair */
	int	value;					/* Value of pair */
} Defaults_pairs;


/*
 ***********************************************************************
 *			Globals
 ***********************************************************************
 */

/*
 * Public Functions
 *
 *
 * NOTE: Any returned string pointers should be considered temporary at best.
 * If you want to hang onto the data, make your own private copy of the string!
 */

/*
 * defaults_exists(name, class_name) will return TRUE if a values exists in the database
 * for name, and class_name.
 */

EXTERN_FUNCTION (Bool   xv_defaults_exists, (char * name, char * class_name));

/*
 * xv_defaults_get_boolean(name, class_name, ddefault) will lookup name and class_name in
 * the defaults database and return TRUE if the value is "True", "Yes", "On",
 * "Enabled", "Set", "Activated", or "1".  FALSE will be returned if the
 * value is "False", "No", "Off", "Disabled", "Reset", "Cleared",
 * "Deactivated", or "0".  If the value is none of the above, a warning
 * message will be displayed and Default will be returned.
 */
EXTERN_FUNCTION (Bool 	xv_defaults_get_boolean, (char *name, char *class_name, Bool ddefault));
/*
 * xv_defaults_get_character(name, class_name, ddefault) will lookup name and class_name in
 * the defaults database and return the resulting character value.  Default
 * will be returned if any error occurs.
 */
EXTERN_FUNCTION (char 	xv_defaults_get_character, (char *name, char *class_name, int default_char));
/*
 * xv_defaults_get_enum(name, class_name, pairs) will lookup the value associated
 * with name and class_name, scan the Pairs table and return the associated value.
 * If no match is found, an error is generated and the value associated with
 * last entry (i.e. the NULL entry) is returned.
 */
EXTERN_FUNCTION (int 	xv_defaults_get_enum, (char *name, char *class_name, Defaults_pairs *pairs));
/*
 * xv_defaults_get_integer(name, class_name, ddefault) will lookup name and class_name in
 * the defaults database and return the resulting integer value. Default will
 * be returned if any error occurs.
 */
EXTERN_FUNCTION (int 	xv_defaults_get_integer, (char *name, char *class_name, int ddefault));

/*
 * xv_defaults_get_integer_check(name, class_name, ddefault, mininum, maximum) will
 * lookup name and class_name in the defaults database and return the resulting
 * integer value. If the value in the database is not between Minimum and
 * Maximum (inclusive), an error message will be printed.  Default will be
 * returned if any error occurs.
 */
EXTERN_FUNCTION (int 	xv_defaults_get_integer_check, (char *name, char *class_name, int ddefault, int minimum, int maximum));

/*
 * xv_defaults_get_string(name, class_name, ddefault) will lookup and return the
 * null-terminated string value assocatied with name and class_name in the
 * defaults database.  Default will be returned if any error occurs.
 */
EXTERN_FUNCTION (char *	xv_defaults_get_string, (char *name, char *class_name, char *ddefault));

/*
 * xv_defaults_init_db() initializes the X11 Resource Manager.
 */
EXTERN_FUNCTION (void 	xv_defaults_init_db, (void));

/*
 * xv_defaults_load_db(filename) will load the server database if filename is
 * NULL, or the database residing in the specified filename.
 */
EXTERN_FUNCTION (void 	xv_defaults_load_db, (char *filename));

/*
 * xv_defaults_store_db(filename) will write the defaults database to the
 * specified file, and update the server Resource Manager property.
 */
EXTERN_FUNCTION (void 	xv_defaults_store_db, (char *filename));

/*
 * xv_defaults_lookup(name, pairs) will linearly scan the Pairs data structure
 * looking for Name.  The value associated with Name will be returned.
 * If Name can not be found in Pairs, the value assoicated with NULL will
 * be returned.  (The Pairs data structure must be terminated with NULL.)
 */
EXTERN_FUNCTION (int 	xv_defaults_lookup, (char *name, Defaults_pairs *pairs));

/*
 * xv_defaults_set_character(resource, value) will set the resource to
 * value.  value is an character. resource is a string.
 */
EXTERN_FUNCTION (void 	xv_defaults_set_character, (char *resource, int value));

/*
 * xv_defaults_set_character(resource, value) will set the resource to
 * value.  value is a integer. resource is a string.
 */
EXTERN_FUNCTION (void 	xv_defaults_set_integer, (char *resource, int value));

/*
 * xv_defaults_set_boolean(resource, value) will set the resource to
 * value.  value is a Boolean. resource is a string.
 */
EXTERN_FUNCTION (void 	xv_defaults_set_boolean, (char *resource, Bool value));

/*
 * xv_defaults_set_string(resource, value) will set the resource to
 * value.  value is a string. resource is a string.
 */
EXTERN_FUNCTION (void 	xv_defaults_set_string, (char *resource, char *value));

#endif /* !AUDIOTOOL_XV_DEFAULTS_H */
