
/* @(#)@(#)ce_int.h	@(#)ce_int.h	3.4 03/22/94
Classing Engine internal data structures
*/

#ifndef ce_int_hdr_DEFINED
#define ce_int_hdr_DEFINED

#define BYTE unsigned char
#define BOOLEAN unsigned char


#include <sys/types.h>
#include "ce_defns.h"

#define CE_MAX_NS 10
#define CE_MAX_ATTR 50
#define CE_MAX_AV 20
#define CE_MAX_ENT 1000

/* Name Space handle  - an index in the global table of name spaces */
typedef CE_NS *CE_NAMESPACE;

/* Entry handle */
typedef CE_ENT *CE_ENTRY;

/* Attribute handle */
typedef CE_ATTR *CE_ATTRIBUTE;


typedef enum {start_all, start_ns, in_match_attrs, in_ns_attrs, in_ns_entries,
		start_new_entry, in_an_entry} BUILD_STATES;

/* define some attr types */
#define CE_OPAQUE_ATTR 0 /* type id for opaque attribute values */
#define CE_MATCH_ATTR 1  /* type id for match attribute values */
#define CE_REF_ATTR 2    /* type id for reference attribute values */

/* the attribute name for the namespace manager attribute */
		static char mgr_attr_name [] = "NS_MANAGER";

		extern CE_NAMESPACE ce_get_namespace_id();
		extern CE_NAMESPACE ce_get_namespace_id_from_db();
		extern CE_ATTRIBUTE ce_get_attribute_id();



#define NS_ATTR_ENT_ID 0   /* entry id for entry that contains a namespace's attrs */

extern int num_ns;
/* Global Name Space array */
extern CE_NS ns_arr[];

/* global indices to various arrays */
extern int curr_NS;
extern int curr_ENT;
extern int curr_ATTR;
extern int curr_AV;


extern BUILD_STATES curr_state;


#endif ce_int_hdr_DEFINED
