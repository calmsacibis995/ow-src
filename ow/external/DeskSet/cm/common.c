#ifndef lint
static  char sccsid[] = "@(#)common.c 1.28 93/05/03 Copyr 1991 Sun Microsystems, Inc.";
#endif

	/* common.c */
/*
 *  Copyright (c) 1987-1990 Sun Microsystems, Inc.
 *  All Rights Reserved.
 *
 *  Sun considers its source code as an unpublished, proprietary
 *  trade secret, and it is available only under strict license
 *  provisions.  This copyright notice is placed here only to protect
 *  Sun in the event the source is deemed a published work.  Dissassembly,
 *  decompilation, or other means of reducing the object code to human
 *  readable form is prohibited by the license agreement under which
 *  this code is provided to the user or company in possession of this
 *  copy.
 *
 *  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *  Government is subject to restrictions as set forth in subparagraph
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *  NASA FAR Supplement.
 */

#include <xview/font.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/notice.h>
#include <xview/scrollbar.h>
#include "util.h"
#include "appt.h"
#include "graphics.h"
#include "calendar.h"
#include "editor.h"
#include "timeops.h"
#include "datefield.h"
#include "props.h"
#include "browser.h"
#include "blist.h"
#include "namesvc.h"
#include "common.h"
#include "gettext.h"
#include "misc.h"
#include "alist.h"
#include "todo.h"

#define NOTICE_CANCEL 2

extern char* periodstr[];

extern void
reset_values(c)
        Calendar *c;
{
        int  hr, end_hr;
        Editor *e = (Editor*)c->editor;
        Props *p = (Props*)c->properties;
        Browser *b = (Browser*)c->browser;
 
        format_tick(b->begin_hr_tick, Order_MDY,
                        Separator_Slash, e->dateval);
        hr = hour(b->begin_hr_tick);
        e->ampmval = e->minhrval = false;
        e->timeval[0] = '\0';
        e->durval[0] = '\0';
        end_hr = hr+1;
        if (p->default_disp_VAL == hour12) {
                if (!adjust_hour(&hr))
                        e->ampmval = true;
                sprintf(e->timeval, "%d:00", hr);
                if (!adjust_hour(&end_hr))
                        e->minhrval = true;
                sprintf(e->durval, "%d:00", end_hr);
        }
        else {
                sprintf(e->timeval, "%02d00", hr);
                if (end_hr >= 24) end_hr -= 24;
                sprintf(e->durval, "%02d00", end_hr);
        }

        e->what[0].itemval[0] ='\0';
        e->what[1].itemval[0] ='\0';
	e->what[2].itemval[0] ='\0';
        e->what[3].itemval[0] ='\0';
	if (e->toval != NULL) free(e->toval);
	e->toval = NULL;
}

extern void
backup_values(c)
        Calendar *c;
{
        int i;
        char *val;
        Editor *d = (Editor*)c->editor;
        Props *p = (Props*)c->properties;

        if (!editor_showing(d)) {
                if (browser_exists(c->browser))
                        reset_values(c);
                return;
        }
	if (!editor_exists(d)) return;

        for (i=0; i<5; i++) {
                cm_strcpy(d->what[i].itemval,
                (char *) xv_get(d->what[i].item, PANEL_VALUE));
        }

        cm_strcpy(d->dateval, (char *) get_date_str(p, d->datetext));

        cm_strcpy(d->timeval, (char *) xv_get(d->time, PANEL_VALUE));

        d->ampmval = (int) xv_get(d->ampm, PANEL_VALUE);

        cm_strcpy(d->durval, (char *) xv_get(d->duration, PANEL_VALUE));

        d->minhrval = (int) xv_get(d->minhr, PANEL_VALUE);
	
	d->periodval = pstr_to_units((char*)xv_get(d->periodunit, PANEL_LABEL_STRING), &d->nthval);

        d->privacyval = privacystr_to_int((char*)xv_get(d->privacyunit, PANEL_LABEL_STRING));
        val = (char *) xv_get(d->scope, PANEL_VALUE);
        if (d->scopeval != NULL) free(d->scopeval);
        d->scopeval = cm_strdup(val);
	if (d->toval != NULL) free(d->toval);
        d->toval= cm_strdup(xv_get(d->to, PANEL_VALUE));
}

extern int
get_data_version(target)
	char *target;
{
        char *calloc;
	int dv = 0;

        calloc = (char*)cm_target2location(target);
        dv = table_version(calloc);
        free(calloc);
	return dv;
}

#if LATER
static void
free_cd(cd)
	Cdata *cd;
{
	free(cd->target);
	free(cd);
}

extern void
free_cd_from_blist(table, row) 
        Panel_item table;  
        int row; 
{ 
        Cdata *cd=NULL; 
 
        cd = (Cdata*)xv_get(table, PANEL_LIST_CLIENT_DATA, row); 
	if (cd != NULL) {
		free_cd(cd);
		xv_set(table, PANEL_LIST_CLIENT_DATA, row, 0, NULL);
	}

}

/* this is for the remote case where there is no entry for the
person in the browse setup list */
static Cdata*
get_static_cdata(name)
        char *name;
{
        static Cdata *cd;

	if (cd == NULL) 
        	cd = (Cdata*)ckalloc(sizeof(Cdata));
	else
		free(cd->target);

	map_name(name, &cd->target);
	cd->data_version = get_data_version(cd->target);

        return cd;
}

static Cdata*
get_new_cdata(name)
        char *name;
{
        Cdata *cd;

        cd = (Cdata*)ckalloc(sizeof(Cdata));
	map_name(name, &cd->target);
	cd->data_version = get_data_version(cd->target);

        return cd;
}

extern Cdata*
get_cdata(bl, name)
	Browselist *bl;
	char *name;
{
        int position=-1;
	Cdata *cd = NULL;

	if (bl != NULL) {
		list_in_list(bl->list, name, &position);
		if (position != -1) {
			cd = (Cdata*)list_client_data(bl->list, position);
			if (cd == NULL) {
				cd = get_new_cdata(name);
				xv_set(bl->list, 
					PANEL_LIST_CLIENT_DATA, 
					position, cd, NULL);
			}
		}
		else 
			cd = get_static_cdata(name);
	}
	else 
		cd = get_static_cdata(name);
	return cd;
}

extern void
map_name(name, target)
        char *name;
        char **target;
{
	*target = NULL;
#if 0 /* change this when we go with mapped names */
#ifdef SVR4
        cm_get_yptarget(name, target);
#endif
#endif
        if (*target == NULL)
                /* No entry in NIS+ db or 4.x */
                *target = cm_strdup(name);
}
#endif /* if LATER */

extern void
blist_write_names(c, list)
        Calendar *c;
	Panel_item list;
{
        int     bufsiz = BUFSIZ;
        int     bufused = 0, i = 0, j = 0;
        char    *buf = (char *) ckalloc(bufsiz);
        char    *name;

        for (i = xv_get(list, PANEL_LIST_NROWS); j < i; j++) {
                name = (char*)list_get_entry(list, j);
                /* see if we have enough space to add it.  If not
                then realloc the buffer bigger */
                if ((bufused + cm_strlen(name) + 3) > bufsiz) 
			buf = (char*)realloc(buf,
                                (unsigned)(bufsiz += BUFSIZ));
                (void) cm_strcat(buf, name);
                (void) cm_strcat(buf, " ");
                bufused += cm_strlen(name) + 1;
        }
        buf[cm_strlen(buf)-1] = '\0';
        cal_update_props();
        cal_set_property(property_names[CP_DAYCALLIST], buf);
        cal_set_properties();
	make_browse_menu(c);
        free(buf);
}
extern void
reset_blist_menu(c, cloc, defcal)
        Calendar *c;
	char *cloc, *defcal;
{
        int  bufsiz = BUFSIZ, bufused = 0;
        char *name, *un, *buf = (char *)ckalloc(bufsiz);
	char *tmp, *namelist, *old_cloc=NULL;
	Props *p = (Props*)c->properties;

	old_cloc = (char*)ckalloc(cm_strlen(un = 
		cm_get_uname()) + cm_strlen(cloc)+2);
	sprintf(old_cloc, "%s@%s", un, cloc);
	tmp = cm_get_property(property_names[CP_DAYCALLIST]);
        namelist =  (char*)ckalloc(cm_strlen(tmp)+1);
        cm_strcpy(namelist, tmp);
	cm_strcat(buf, c->calname);
	cm_strcat(buf, " ");
	bufused += cm_strlen(c->calname) + 1;
	if (strcmp(c->calname, p->defcal_VAL) != 0) {
		cm_strcat(buf, p->defcal_VAL);
		cm_strcat(buf, " ");
		bufused += cm_strlen(p->defcal_VAL) + 1;
	}

	if (namelist != NULL && *namelist != NULL) {
                name = (char *)strtok(namelist, " ");
                while (name != NULL && *name != NULL) {
                        if ((old_cloc && strcmp(old_cloc, name) == 0)
			 || (defcal && strcmp(defcal, name) == 0)) {
                                name = (char *) strtok((char *)NULL, " ");
                                continue;
			}
                	/* see if we have enough space to add it.  
			If not then realloc the buffer bigger */
                	if ((bufused + cm_strlen(name) + 3) > bufsiz) 
			buf = (char*)realloc(buf,
                                (unsigned)(bufsiz += BUFSIZ));
                	(void) cm_strcat(buf, name);
                	(void) cm_strcat(buf, " ");
                	bufused += cm_strlen(name) + 1;
                	name = (char *)strtok((char*)NULL, " ");
        	}
	}
        buf[cm_strlen(buf)-1] = '\0';
        cal_update_props();
        cal_set_property(property_names[CP_DAYCALLIST], buf);
        cal_set_properties();
	make_browse_menu(c);
        free(buf);
}

extern void
reset_alarm(c)
	Calendar *c;
{
	if (c->view->next_alarm != NULL) {
		destroy_reminder(c->view->next_alarm);
		c->view->next_alarm=NULL;
	}
	reset_timer();
}

/* real name stored as client data  */
extern Boolean
duplicate_cd(box, name, num)
        Panel_item box;
        char *name;
        int *num;
{
        int i;
        char *list_name;

        *num = -1;
        for (i = xv_get(box, PANEL_LIST_NROWS)-1; i >= 0; i--) {
                list_name = (char*)xv_get(box, PANEL_LIST_CLIENT_DATA, i);
                if (strcmp(name, list_name) == 0) {
                        *num = i;
                        return true;
                }
        }
        return false;
}

extern char *
get_appt_str()
{
	Props *p = (Props*)calendar->properties;
        Browser *b = (Browser*)calendar->browser;
        Editor *e = (Editor*)calendar->editor;
	char *whatstr;
        char buf[BUFSIZ], date_buf[100], start_hour[10], end_hour[10];
        char what_buf[BUFSIZ], repeat_buf[40], for_buf[20];
	Boolean show = false;
	int i, nthval, wk, alert_res;
	Interval periodval = (Interval)e->periodval;
 
	what_buf[0] = '\0';
	for_buf[0] = '\0';
        if (show = editor_showing(e)) {
		cm_strcpy(date_buf, (char*)get_date_str(p, e->datetext));
		cm_strcpy(start_hour, (char *)xv_get(e->time, PANEL_VALUE));
		if (start_hour[0] != '\0') {
			if (p->default_disp_VAL == hour12)
				cm_strcat(start_hour,
					xv_get(e->ampm, PANEL_VALUE)?
					"pm" : "am" );	
		}
		cm_strcpy(end_hour, (char *)xv_get(e->duration, PANEL_VALUE));
		if (end_hour[0] != '\0') {
			if (p->default_disp_VAL == hour12)
				cm_strcat(end_hour,
					xv_get(e->minhr, PANEL_VALUE)? 
					"pm" : "am" );
		}
                for(i = 0; i < 5; i++) {
			whatstr = (char *)xv_get(e->what[i].item, PANEL_VALUE);
			if (whatstr && !blank_buf(whatstr)) {
                        	cm_strcat (what_buf, whatstr);
                        	cm_strcat (what_buf, "\n\t");
			}
		}

		cm_strcpy(repeat_buf,
			period_to_Clocstr(e->periodval, e->nthval));
		if (e->periodval)
			sprintf(for_buf, "\tFor: %s\n",
				(char *)xv_get(e->scope, PANEL_VALUE));

	} else if (!show) {
		cm_strcpy(date_buf, e->dateval);
		cm_strcpy(start_hour, e->timeval);
		if (start_hour[0] != '\0') {
			if (p->default_disp_VAL == hour12)
				cm_strcat(start_hour,
					e->ampmval ? "pm" : "am" );
		}
		cm_strcpy(end_hour, e->durval);
		if (end_hour[0] != '\0') {
			if (p->default_disp_VAL == hour12)
				cm_strcat(end_hour,
					e->minhrval ? "pm" : "am" );
		}
		for(i = 0; i < 5; i++) {
			if (!blank_buf(e->what[i].itemval))
			{
				cm_strcat (what_buf, e->what[i].itemval);
				cm_strcat (what_buf, "\n\t");
			}
		}

		cm_strcpy(repeat_buf,
			period_to_Clocstr(e->periodval, e->nthval));
		if (e->periodval)
			sprintf(for_buf, "\tFor: %s\n", e->scopeval);

		if (browser_exists(b))
			reset_values(calendar);
	}

	if (periodval == nthWeekday) {
		sprintf(buf, "%s %s", date_buf, start_hour);
		if (weekofmonth(cm_getdate(buf, NULL), &wk) && wk == 4) {
			alert_res = notice_prompt(calendar->frame,
				(Event *)NULL,
				NOTICE_MESSAGE_STRINGS,
				MGET("Is this appointment in the Last Week of the Month or"),
				MGET("the 4th Week of the Month?"),
				NULL,
				NOTICE_BUTTON_YES, LGET("Last Week"),
				NOTICE_BUTTON_NO, LGET("4th Week"),
				NOTICE_BUTTON, LGET("Cancel"), NOTICE_CANCEL,
				NULL);
			if (alert_res == NOTICE_CANCEL)
				return(NULL);
			else if (alert_res == NOTICE_YES)
				nthval = -1;
			else
				nthval = wk;
		} else if (wk == 5)
			nthval = -1;
		else
			nthval = wk;

		if (nthval == -1)
			strcat(repeat_buf, ", last");
		else if (nthval == 4)
			strcat(repeat_buf, ", 4th");
	}

        sprintf(buf, "\n\n\t** Calendar Appointment **\n\n\tDate: %s\n\tStart: %s\n\tEnd: %s\n\tRepeat: %s\n%s\tWhat: %s ",
			date_buf, start_hour, end_hour,
			repeat_buf, for_buf, what_buf);

	return(cm_strdup(buf));
}
extern void
common_update_lists(c)
        Calendar *c;
{
        Alist *a;
        Todo *t;

        if (alist_showing(c->alist)) {
                a = (Alist*)c->alist;
                a->changed = true;
                a_create_alist(c, a->glance);
        }
        if (todo_showing(c->todo)) {
                t = (Todo*)c->todo;
                t->changed = true;
                t_create_todolist(c, t->glance);
        }
}
