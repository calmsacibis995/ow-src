/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _AUDIOTOOL_DEFAULTS_H
#define	_AUDIOTOOL_DEFAULTS_H

#ident	"@(#)defaults.h	1.8	92/12/14 SMI"

#include <atool_types.h>
#include <atool_i18n.h>

/* for defaults handling */
#define DEFAULTS_APP_NAME			"audiotool"
#define DEFAULTS_APP_CLASS			"Audio"
#define DEFAULTS_APP_FILE			".audiorc"

/* define default types */

#include <sys/types.h>

#define DT_BOOL		0
#define DT_CHAR		1
#define DT_ENUM		2
#define DT_INT		3
#define DT_STR		4
#define DT_FLT		5

#define TMP_DEFAULTS	"/tmp/dfltmpXXXXXX" /* tmp file for writing dflts */

struct defaults_list {
	char *name;		/* name of default */
	char *class;		/* class name */
	char type;		/* type of default (from above) */
	ptr_t val;		/* value (opaque ptr) */
	char *help;		/* help string */
	void (*set_value)();	/* callback to actually set the value
				 * (takes value and client data
				 * as param's)
				 */
	char inst;		/* if TRUE, store instance, else store class */
};

struct defaults_data {
	char 			*app_class; 	/* application class */
	char			*app_name; 	/* application name */
	char			*app_file; 	/* application defaults file */
	ptr_t			client_data;	/* client data for cb's */
	struct defaults_list    *dlp; 		/* defaults list  */
};

extern struct defaults_data *InitDefaults(struct defaults_list *dlp,
                                          char *app_name,
                                          char *app_class,
                                          char *app_file,
                                          ptr_t client_data);
extern int ReadDefaults(struct defaults_data *ddp);
extern int WriteDefaults(struct defaults_data *ddp);
extern ptr_t GetDefault(struct defaults_data *ddp, char *name);
extern int SetDefault(struct defaults_data *ddp, char *name, ptr_t val);

#endif /* !_AUDIOTOOL_DEFAULTS_H */
