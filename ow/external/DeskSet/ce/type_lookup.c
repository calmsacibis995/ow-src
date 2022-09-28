/* @(#)type_lookup.c	1.8 - 90/12/01 */
/*
 * Build and lookup routines for the Type namespace 
 */
#include <varargs.h>
#include "ce.h"

#define NULL 0

int
init_mgr (num_func, func_ptrs)
int	*num_func;
void	***func_ptrs;
{
	void	**funcs;
	int build ();
	int match ();
	int get_entry_cookie ();
	int match_entry_cookie ();
	
	*num_func = 4;
	funcs = (void **) calloc (4, sizeof (int (*) ()));

	if (!funcs) return (0);
	

	funcs [0] = (void *)build;
	funcs [1] = (void *)match;
	funcs [2] = (void *)get_entry_cookie;
	funcs [3] = (void *)match_entry_cookie;
	*func_ptrs = funcs;
	
	return (1);
}
	
static int
build (namespace, entry, new_buffer, new_size)
CE_NAMESPACE namespace;
CE_ENTRY entry;
void **new_buffer;
int *new_size;
{
	static char type_attr_name [] = "TYPE_NAME";
	CE_ATTRIBUTE attribute;
	void *tmp_ptr;
	int tmp_size;
  
	/* get the TYPE_NAME attribute for this entry */
	attribute = ce_get_attribute_id (namespace, "TYPE_NAME");
	*new_buffer = (void *) ce_get_attribute (namespace, entry, attribute);
	*new_size = 0; /* indication to CE not to free this memory */
	return (0);
}

/* match routine */
static int
match (argcount, args, matchcount, match_val, entry, ret_value)
int argcount;
va_list args;
int matchcount;
void **match_val;
CE_ENTRY entry;
void **ret_value;
{
	char *in_type_name, *match_type_name;
	int cmp_res;
	/* check if all arguments are kosher - if not, get the hell out */
	if (argcount != 1)
		return (-1);
	if ((in_type_name = (char *)va_arg (args, char *)) == NULL)
		return (-1);
	if ((match_type_name = (char *) match_val) == NULL)
		return (-1);	
	if (entry == NULL)
		return (-1);
	if (ret_value == NULL)
		return (-1);

	/* OK, so we're kosher */
	cmp_res = strcmp (in_type_name, match_type_name);
	
	if (cmp_res == 0)
		*ret_value = (void *) entry;
	return cmp_res;

}

/* get entry cookie */
static int
get_entry_cookie (namespace, entry, cookie_ptr)
CE_NAMESPACE namespace;
CE_ENTRY entry;
char **cookie_ptr;
{
	char	*result;
	char	*type_name_val;
	static char type_attr_name [] = "TYPE_NAME";
	CE_ATTRIBUTE attribute;

	/* get the TYPE_NAME attribute for this entry */
	attribute = ce_get_attribute_id (namespace, type_attr_name);
	type_name_val = (char *) ce_get_attribute (namespace, entry, attribute);
	if (type_name_val)	{
		result = (char *)strdup (type_name_val);
		*cookie_ptr = result;
		return (1);
	}
	else
		return (0);
	
}

/* match entry cookie */
static int
match_entry_cookie (namespace, cookie, entry)
CE_NAMESPACE namespace;
char *cookie;
CE_ENTRY entry;
{
	static char type_attr_name [] = "TYPE_NAME";
	CE_ATTRIBUTE attribute;
	char	*entry_cookie;
	
	/* get the TYPE_NAME attribute for this entry */
	attribute = ce_get_attribute_id (namespace, type_attr_name);
	if (entry_cookie = ce_get_attribute (namespace, entry, attribute))
		return (strcmp (entry_cookie, cookie));
	return (-1);
}


  

