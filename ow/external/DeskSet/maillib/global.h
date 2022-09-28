/* @(#)global.h	3.2 - 94/07/29 */

struct globals {
	char *g_myname;
	void *g_ignore;		/* hash list of ignored fields */
	void *g_retain;		/* hash list of retained fields */
	void *g_alias;		/* hash list of alias names */
	void *g_alternates;	/* hash list of alternate names */
	int g_nretained;	/* the number of retained fields */
	char *g_iconvinfo;   /* for mime translation */
};

extern struct globals glob;
