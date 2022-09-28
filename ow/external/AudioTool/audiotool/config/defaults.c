/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)defaults.c	1.7	92/12/14 SMI"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/param.h>
#include <string.h>
#include <xview/xview.h>

#include "atool_debug.h"
#include "xv_defaults.h"	/* use local routines */
#include "defaults.h"

struct defaults_data *
InitDefaults(
	struct defaults_list	*dlp,
	char			*app_name,
	char			*app_class,
	char			*app_file,
	ptr_t			client_data)
{
	char			path[MAXPATHLEN+1];
	struct defaults_data	*ddp; 

	if (!(ddp = (struct defaults_data *)
	      calloc(1, sizeof (struct defaults_data)))) {
		return (struct defaults_data *)NULL;
	}

	ddp->app_name = app_name;
	ddp->app_class = app_class;
	ddp->app_file = app_file;
	ddp->client_data = client_data;
	ddp->dlp = dlp;

	/* only read them if a file is specified ... */
	if (ddp->app_file && *ddp->app_file) {
		sprintf(path, "%s/%s", getenv("HOME"), ddp->app_file);
		xv_defaults_load_db(path);
	}
	return (ddp);
}

/* read defaults file and fill in default list ... */

ReadDefaults(struct defaults_data *ddp)
{
	register struct defaults_list *dlp;
	char rname[100], cname[100];
	char *sval;
	int ival;
	Bool bval;

	for (dlp = ddp->dlp; dlp && dlp->name && *dlp->name; dlp++) {
		sprintf(rname, "%s.%s", ddp->app_name, dlp->name);
		sprintf(cname, "%s.%s", ddp->app_class, dlp->class);

		switch (dlp->type) {
		case DT_INT:
			ival = xv_defaults_get_integer(rname, cname, 
						       (int)dlp->val);
			dlp->val = (ptr_t) ival;
			break;
		case DT_STR:
			sval = xv_defaults_get_string(rname, cname, 
						      (char *)dlp->val);
			dlp->val = (ptr_t) ((sval && *sval) ?
					    strdup(sval) : NULL);
			break;
		case DT_BOOL:
			bval = xv_defaults_get_boolean(rname, cname,
						       (Bool)dlp->val);
			dlp->val = (ptr_t) bval;
			break;
		case DT_ENUM:	/* not yet */
		case DT_CHAR:	/* not yet */
		default:
			break;
		}
		if (dlp->set_value) {
			(*dlp->set_value)(ddp->client_data, dlp->val);
		}
	}    
		
}

/* since defaults_store_db writes out ALL of your resources, write it out 
 * to a tmp file, read it back and filter out all application specific
 * resources (HACK ATTACK!).
*/
WriteDefaults(
	struct defaults_data	*ddp)
{
	char			path[MAXPATHLEN+1];
	char			bpath[MAXPATHLEN+1];
	char			tmppath[MAXPATHLEN+1];
	char			buf[BUFSIZ];
	char			*cp;
	FILE			*tmpfp;
	FILE			*fp;
    
	/* if no default's file name set, just return */
	if (! (ddp->app_file && *ddp->app_file))
	    return;

	sprintf(path, "%s/%s", getenv("HOME"), ddp->app_file);
	strncpy(bpath, path, MAXPATHLEN);
	strncat(bpath, ".BAK", MAXPATHLEN);
    
	unlink(bpath);

	if ((rename (path, bpath) == -1) && (errno != ENOENT)) {
		DBGOUT((1, "WriteDefaults: can't rename %s to %s\n",
		    path, bpath));
		unlink(path);
		return (-1);
	}
	xv_defaults_store_db(path);
}

ptr_t
GetDefault(struct defaults_data *ddp, char *name)
{
	register struct defaults_list *dlp;

	for (dlp = ddp->dlp ; dlp && dlp->name && *dlp->name; dlp++) {
		if ((strcmp(dlp->name, name)==0) 
		    || (strcmp(dlp->class, name)==0)) {
			return (dlp->val);
		}
	}
	return (NULL);
}

/* SetDefault: return 1 if value changed, 0 if it hasn't or on error */

SetDefault(struct defaults_data *ddp, char *name, ptr_t val)
{
	register struct defaults_list *dlp;
	char def[100];		/* hmm?? */
    
	for (dlp = ddp->dlp ; dlp && dlp->name && *dlp->name; dlp++) {
		if ((strcmp(dlp->name, name)==0) 
		    || (strcmp(dlp->class, name)==0)) {
			break;
		}
	}

	if (!(dlp && dlp->name && *dlp->name)) {
		DBGOUT((1, "** can't find default %s\n", name));
		return (0);
	}

	if (dlp->inst == TRUE) {
		/* store instance name */
		sprintf(def, "%s.%s", ddp->app_name, dlp->name);
	} else {
		sprintf(def, "%s.%s", ddp->app_class, dlp->class);
	}

	switch (dlp->type) {
	case DT_STR:
		if (dlp->val && (char*)val && *(char *)val) {
			if (strcmp((char *)val, dlp->val) == 0) {
				/* NOT CHANGED */
				return (0);
			}
		}
		if ((char *)dlp->val) {
			free((char *)dlp->val);
		}

		if ((char*)val && *(char*)val) {
			dlp->val = (ptr_t) strdup(val);
			xv_defaults_set_string(def, (char*)dlp->val);
		} else {
			dlp->val = NULL;
		}
		break;

	case DT_INT:
		if (val == dlp->val) {
			/* NOT CHANGED */
			return (0);
		}

		dlp->val = val;
		xv_defaults_set_integer(def, (int)dlp->val);
		break;

	case DT_BOOL:
		if (val == dlp->val) {
			/* NOT CHANGED! */
			return (0);
		}

		dlp->val = val;
		xv_defaults_set_boolean(def, (int)dlp->val);
		break;

	default:
		dlp->val = val;
	}

	/* if a set_value callback is defined, call it with the view as the
	   client data and the new value.
	   */

	if (dlp->set_value)
	    (*dlp->set_value)(ddp->client_data, dlp->val);
	return (1);
}

void SetDefaultCallback(struct defaults_data *ddp,
                        char *name,
                        void (*call_back) (/*XXX*/))
{
	register struct defaults_list *dlp;

	for (dlp = ddp->dlp ; dlp && dlp->name && *dlp->name; dlp++) {
		if ((strcmp(dlp->name, name)==0) 
		    || (strcmp(dlp->class, name)==0)) {
			dlp->set_value = call_back;
			break;
		}
	}
}
