/* @(#)fns.h	@(#)fns.h	3.1 04/03/92 */

/* ns.h : definitions for the file name space lookup */


enum file_ns_literals {
	FNSL_FILENAME,
	FNSL_MAG_OFFSET,
	FNSL_MAG_TYPE,
	FNSL_MAG_MASK,
	FNSL_MAG_MATCH,
	FNSL_MAG_OPS,
	FNSL_DIRNAME,
	FNSL_SIZE
};

enum magic_types {
	MT_NONE,
	MT_BYTE,
	MT_SHORT,
	MT_LONG,
	MT_STRING
};

enum magic_ops {
	MO_EQUAL,	/* =, number must match exactly */
	MO_GT,		/* >, number must be greater than match */
	MO_LT,		/* <, number must be less than match */
	MO_ALL,		/* &, all bits in match must be set */
	MO_EXCLUDE	/* ^, at least one bit in match must not be set */
};

