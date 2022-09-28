#ifndef lint
static  char sccsid[] = "@(#)defnamesvc.c 1.8 92/11/23 Copyr 1991 Sun Microsystems, Inc.";
#endif
/* defnamesvc.c */

#include <stdio.h>
#include <xview/xview.h>
#include <xview/font.h>
#include <xview/panel.h>
#include "util.h"
#include "graphics.h"
#include "calendar.h"
#include "timeops.h"
#include "datefield.h"
#include "props.h"
#include "defnamesvc.h"

/* 
These routines set and get the calendar name 
*/

/* Routine sets the calendar name */
extern void
cm_set_deftarget()
{
	char *uname;
	Props *p = (Props*)calendar->properties;

	if (p->cloc_VAL == NULL) 
		strcpy(p->cloc_VAL, cm_get_local_host());
	free(calendar->calname);
        uname = (char*)cm_get_uname();
	calendar->calname = (char*)ckalloc(cm_strlen(uname) +
				 cm_strlen(p->cloc_VAL) + 2);
	sprintf(calendar->calname, "%s@%s", uname, p->cloc_VAL);
}

/* Calling routine responsible for freeing returned memory */
/* Returns default calendar */
extern char*
cm_get_deftarget()
{
        char *name, *uname, *loc, *dc, *tail=NULL;
	Props *p = (Props*)calendar->properties;
 
	uname = cm_get_uname();
	loc = cm_get_property(property_names[CP_CALLOC]);
	if (loc == NULL || *loc == NULL) {
		loc = (char*)cm_get_local_host();
		/* For backward compatibility; we assume that user was
		setting props def cal setting to set User calendar name */
		dc = cm_get_property(property_names[CP_DEFAULTCAL]);
		if (dc != NULL) {
			name = cm_target2name(dc);
			if (strcmp(name, uname) == 0) {
				tail = get_tail(dc, '@');
				if (tail != NULL)
					loc = tail;
			}
			free(name);
		}
	}
	name = ckalloc(cm_strlen(uname) + cm_strlen(loc) + 2);
	sprintf(name, "%s@%s", uname, loc);
	if (tail != NULL)
		free(tail);

        return name;
}
