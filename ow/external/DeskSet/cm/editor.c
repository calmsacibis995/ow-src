#ifndef lint
static  char sccsid[] = "@(#)editor.c 3.101 96/06/19 Copyr 1991 Sun Microsystems, Inc.";
#endif
/* editor.c */

#include <stdio.h>
#include <ctype.h>
#include <xview/xview.h>
#include <xview/notice.h>
#include <xview/panel.h>
#include <xview/font.h>
#include <xview/textsw.h>
#include <xview/scrollbar.h>
#include <rpc/types.h> /* need for 4.0 compatibility */
#include <ds_listbx.h>
#include <ds_popup.h>
#include <string.h>
#include "util.h"
#include "graphics.h"
#include "timeops.h"
#include "appt.h"
#include "table.h"
#include "datefield.h"
#include "props.h"
#include "editor.h"
#include "browser.h"
#include "calendar.h"
#include "select.h"
#include "gettext.h"
#include "common.h"
#include "todo.h"
#include "alist.h"
#include "repeat.h"
#include "misc.h"
#ifdef SVR4
#include <sys/param.h>
#endif "SVR4"

Xv_Font box_font;

#define	BEEP	0
#define	FLASH	1
#define OPEN	2
#define MAIL	3

#define bp_mask 1
#define fl_mask 2
#define op_mask 4
#define ml_mask 8

#define		NONE		 MGET("No Time") 

#define LISTENTRYLEN        31
#define choice_on(value, bit)	((value) &(1 <<(bit)))

#define NOTICE_CANCEL 69
#define NOTICE_FORWARD 70

static Notify_value e_insert_proc(), e_delete_proc(), e_change_proc();
static void apptbox_action();
static void add_to_apptbox(), select_appt_and_reset_calbox();
static void update_mailto();

static Boolean datefilled, startfilled, whatfilled, dnd_full;

Appt *ca;	/* current appointment */
extern void reset_timer();
Uid *interest_list=NULL;
extern char* periodstr[], *unitstr[], *privacystr[];

struct apptowners {
	char *name;
	char *mailto;
	Uid  *uid;
	struct apptowners *next;
	struct apptowners *prev;
}; 
struct drawind {
	int start, end;
	struct drawind *next;
}; 
struct drawind *index_list=NULL;

extern Boolean
editor_exists(e)
        Editor *e;
{
        if (e != NULL && e->frame != NULL)
                return true;
        return false;
}
extern Boolean
editor_showing(e)
        Editor *e;
{
        if (e != NULL && e->frame != NULL && xv_get(e->frame, XV_SHOW))
                return true;
        return false;
}

static void
set_dnd_target()
{
	Editor *e = (Editor *)calendar->editor;

	if (!editor_exists(e)) return;
	if ((datefilled && startfilled) || (datefilled && whatfilled)) {
		if (dnd_full == false) {
			xv_set(e->dnd_target, PANEL_DROP_FULL, TRUE, NULL);
			dnd_full = true;
		}
	} else if (dnd_full) {
		xv_set(e->dnd_target, PANEL_DROP_FULL, FALSE, NULL);
		dnd_full = false;
	}
}

#define STR_LEN 20
static void
add_calbox_entry(c, font, name, str, select)
        Calendar *c;
        Xv_Font  font;
        char     *name, *str;
	Boolean select; /* should the entry be selected? */
{
	char    access_entry[LISTENTRYLEN], *tmp=NULL;
        int     i, row = 0;
	Editor *e = (Editor*)c->editor;

        access_entry[0] = '\0';
        strncpy (access_entry, name, STR_LEN);
        access_entry[STR_LEN] = '\0';
        for (i = cm_strlen(access_entry); i <= STR_LEN; i++)
                access_entry[i] = ' ';
        access_entry[i] = '\0';
        cm_strcat (access_entry, str);
        /* must cm_strdup(name) cuz it is toolkit memory and 
	we're storing it here as client data. Gets freed when 
	the list gets flushed. */
	row = xv_get(e->calbox, PANEL_LIST_NROWS);
        list_add_entry_font(e->calbox, access_entry, cm_strdup(name), 
			row, font, TRUE);
	if (select) {
		list_entry_select_n(e->calbox, row);
		if (!name_in_list(c, name, xv_get(e->to, PANEL_VALUE), &tmp))
			update_mailto(c);
		if (tmp != NULL) free(tmp);
	}
}
extern void
update_mailto_entry(c, name)
	Calendar *c;
	char *name;
{
	Editor *e = (Editor*)c->editor;
	char *tmp=NULL;

	if (name_in_list(c, name, xv_get(e->to, PANEL_VALUE), &tmp))
		update_mailto(c);
	if (tmp != NULL) free(tmp);
}
extern void
add_to_calbox(c, name)
	Calendar *c;
	char *name;
{
	Editor *e = (Editor*)c->editor;
	Access_Entry    *list = NULL;
	extern void destroy_access_list();
	Stat stat;
	int num;
	char *uname;

	if (duplicate_cd(e->calbox, name, &num)) return;

	stat = table_get_access(name, &list);
	uname = cm_target2name(name);
        if (strcmp(cm_get_uname(), uname) == 0) {
		if (source_has_access(list, c->user))
			add_calbox_entry(c, box_font, name, MGET("Y"), true);
		else
			add_calbox_entry(c, box_font, name, MGET("N"), true);
		free(uname);
		return;
	}
	free(uname);


	if (list == NULL) {
		if (stat == status_ok)
		 	add_calbox_entry(c, box_font, name, MGET("N") , true);
		else
		 	add_calbox_entry(c, box_font, name, MGET("?") , true);

		return;

	}

	if (access_write & user_permission(list, c->calname))
		add_calbox_entry(c, box_font, name, MGET("Y"), true);
	else
		add_calbox_entry(c, box_font, name,  MGET("N") , false);
	destroy_access_list(list);
}
extern void
set_default_mail(c)
	Calendar *c;
{
	Editor *e = (Editor*)c->editor;
	Props *p = (Props*)c->properties;

	if (!editor_exists(e)) 
		make_editor(c);
	if (strcmp(c->view->current_calendar, c->calname) == 0)
		xv_set(e->to, PANEL_VALUE, p->mailto_VAL, NULL);
	else
		xv_set(e->to, PANEL_VALUE, 
			c->view->current_calendar, NULL);
}
extern void
set_default_calbox(c)
	Calendar *c;
{
	Editor *e;
	Props *p;

	e = (Editor*)c->editor;
	if (!editor_exists(e)) 
		make_editor(c);
	p = (Props*)c->properties;
	list_cd_flush(e->calbox);
	add_to_calbox(c, c->view->current_calendar);
	set_default_mail(c);
}
extern void
browser2calbox(c)
	Calendar *c;
{
	int i;
	char *name;
	Browser *b = (Browser*)c->browser;
	Editor *e = (Editor*)c->editor;

	if (!editor_exists(e)) return;
	list_cd_flush(e->calbox);
        i = xv_get(b->box, PANEL_LIST_FIRST_SELECTED);
        while (i != -1) {
                name = (char*)xv_get(b->box, PANEL_LIST_STRING, i);
                add_to_calbox(c, name);
                i = xv_get(b->box, PANEL_LIST_NEXT_SELECTED, i);
        }
}
extern void
add_times(box)
	Panel_item box;
{
	int k, i, j;
	char *buf, *name, *date = NULL;
	Appt *a=NULL, *head=NULL;
	Editor *e;
	Props *p;
	Calendar *c = calendar;
	Range range;
	Lines *lines=NULL;
	char date_str[256];

	e = (Editor *) c->editor;
	if (!editor_exists(e)) return;
	p = (Props *) c->properties;

	cm_strcpy(date_str, date = (char*)get_date_str(p, e->datetext));
	if ( date == NULL )
		return;
	if (strcasecmp(date, "today") == 0 ||
		 strcasecmp(date, "tomorrow") == 0 ||
		 strcasecmp(date, "yesterday") == 0)
		cm_strcat(date_str, "12:00 am");
		
	k = (int) cm_getdate(date_str, NULL);
	if (k <= 0) return;

	xv_set(box, XV_SHOW, FALSE, NULL);
	e_list_flush(box);

	range.key1 = lowerbound(k);
	range.key2 = next_ndays(k, 1);
	range.next = NULL;

	for (j = 0, i = xv_get(e->calbox, PANEL_LIST_NROWS); j < i; j++) {
		name = (char*)xv_get(e->calbox, PANEL_LIST_CLIENT_DATA, j); 
		table_lookup_range(name, &range, &a);
		head = a;
		while(a != NULL && a->appt_id.tick < range.key2) { 
			lines = text_to_lines(a->what,1);
			if (lines != NULL && lines->s != NULL) {
				buf = (char *) ckalloc(cm_strlen(lines->s) + 18); 
				destroy_lines(lines); lines=NULL;
			}
			else
				buf = (char *) ckalloc(15); 
			add_to_apptbox(e, a, name, buf);
			a = a->next;
		}
		if (head != NULL) {
			destroy_appt(head); 
			head = NULL;
		}
	}
	xv_set(box, XV_SHOW, TRUE, NULL);
}
/* ARGSUSED */
extern Notify_value
update_handler(me, fd)
        Notify_client me;
        int fd;
{
	Editor *e = (Editor *)calendar->editor;
	Browser *b= (Browser *)calendar->browser;
	Props *p= (Props *)calendar->properties;
	Calendar *c= calendar;

	paint_canvas(calendar, NULL);
	reset_alarm(calendar);

	if (browser_showing(b)) 
		update_browser_display(b, p);

	if (editor_showing(e))
		add_times(e->apptbox);

	common_update_lists(c);

        return(NOTIFY_DONE);
}

extern void
e_list_flush(list)
        Panel_item list;
{
        int     i;
	struct apptowners *ao, *nextao;

	if (ca != NULL) {
		destroy_appt(ca);
		ca = NULL;
	}
        for (i=xv_get(list, PANEL_LIST_NROWS)-1; i >= 0; i--) {
		ao = (struct apptowners *) xv_get(list, PANEL_LIST_CLIENT_DATA, i);
		while (ao != NULL) {
			nextao = ao->next;
			free(ao->name);
			free(ao->mailto);
			free(ao->uid);
			free(ao);
			ao = nextao;
		}
		xv_set(list, PANEL_LIST_DELETE, i, NULL);
        }
}

static Notify_value
e_done_proc(frame)
        Frame frame;
{
	Editor *e = (Editor*)calendar->editor;
	Repeat *r = (Repeat*)e->repeat;

	backup_values(calendar); 
        xv_set(frame, XV_SHOW, FALSE, NULL);
	if (r != NULL && r->frame != NULL && xv_get(r->frame, XV_SHOW))
        	xv_set(r->frame, XV_SHOW, FALSE, NULL);
	
        return(NOTIFY_DONE);
}
extern Boolean
showing_browser(b, tick)
	Browser *b;
	Tick tick;
{
	if (tick >= b->begin_week_tick && 
		tick < next_ndays(b->begin_week_tick, 8))
		return true;
	return false;
}

/* ARGSUSED */
static Notify_value
reminder_notify(item, value, event)
	Panel_item item;
	int	value;
	Event *event;
{
	Calendar *c;
	Editor *e;
	Boolean set_val;

	c = (Calendar *) xv_get(item, PANEL_CLIENT_DATA);
	e = (Editor *) c->editor;

	mail_on(e) ? (set_val=false) : (set_val=true);
	xv_set(e->to, PANEL_INACTIVE, set_val, 0);
	xv_set(e->mailadvance, PANEL_INACTIVE, set_val, 0);
	xv_set(e->mailunit, PANEL_INACTIVE, set_val, 0);
	xv_set(e->mailunitstack, PANEL_INACTIVE, set_val, 0);

	beep_on(e) ? (set_val=false) : (set_val=true);
	xv_set(e->beepadvance, PANEL_INACTIVE, set_val, 0);
	xv_set(e->beepunit, PANEL_INACTIVE, set_val, 0);
	xv_set(e->beepunitstack, PANEL_INACTIVE, set_val, 0);

	flash_on(e) ? (set_val=false) : (set_val=true);
	xv_set(e->flashadvance, PANEL_INACTIVE, set_val, 0);
	xv_set(e->flashunit, PANEL_INACTIVE, set_val, 0);
	xv_set(e->flashunitstack, PANEL_INACTIVE, set_val, 0);

	open_on(e) ? (set_val=false) : (set_val=true);
	xv_set(e->openadvance, PANEL_INACTIVE, set_val, 0);
	xv_set(e->openunit, PANEL_INACTIVE, set_val, 0);
	xv_set(e->openunitstack, PANEL_INACTIVE, set_val, 0);

	return(NOTIFY_DONE);
}

extern void
set_default_reminders(c)
	Calendar *c;
{
	Props *p = (Props*)c->properties;
	Editor *e = (Editor*)c->editor;

	if (!editor_exists(e)) 
		make_editor(c);
	/*      Get reminder defaults from the property sheet   */
	xv_set(e->reminder, PANEL_VALUE, p->reminder_VAL, 0);
	xv_set(e->beepadvance, PANEL_VALUE, p->beepadvance_VAL, 0);
	xv_set(e->flashadvance, PANEL_VALUE, p->flashadvance_VAL, 0);
	xv_set(e->openadvance, PANEL_VALUE, p->openadvance_VAL, 0);
	xv_set(e->mailadvance, PANEL_VALUE, p->mailadvance_VAL, 0);
	xv_set(e->beepunit, PANEL_LABEL_STRING, unitstr[p->beepunit_VAL], 0);
	xv_set(e->flashunit, PANEL_LABEL_STRING, unitstr[p->flashunit_VAL], 0);
	xv_set(e->openunit, PANEL_LABEL_STRING, unitstr[p->openunit_VAL], 0);	
	xv_set(e->mailunit, PANEL_LABEL_STRING, unitstr[p->mailunit_VAL], 0);
	reminder_notify(e->reminder, 0, 0);
	set_default_mail(c);
}
extern void
set_default_scope_plus(e)
	Editor *e;
{
	if (e->scopeval != NULL) free(e->scopeval);
	e->scopeval=ckalloc(1);
	e->privacyval = e->periodval = 0;
	e->nthval = 0;
	e->scopeunitval=repeatstr[0];
	xv_set(e->periodunit, PANEL_LABEL_STRING, periodstr[0], 0);
	xv_set(e->scope, PANEL_VALUE, e->scopeval, 0);
	xv_set(e->scopeunit, PANEL_LABEL_STRING, e->scopeunitval, 0);
	xv_set(e->scope, PANEL_VALUE, repeatval[(int)e->periodval], 0);
	xv_set(e->scopeunit, PANEL_LABEL_STRING, repeatstr[(int)e->periodval], 0);
	activate_scope(e);
	xv_set(e->appt_type, PANEL_VALUE, 0, NULL);
	xv_set(e->frame, FRAME_LEFT_FOOTER, "", 0);
}

static void
set_default_time(e)
	Editor *e;
{
	Props *p = (Props*)calendar->properties;

	if (p->default_disp_VAL == hour12) {
		cm_strcpy(e->timeval, "9:00");
		cm_strcpy(e->durval,  "10:00");	
	}
	else {
		cm_strcpy(e->timeval, "0900");
		cm_strcpy(e->durval,  "1000");	
	}
	e->ampmval=e->minhrval=0;
	e->dateval[0] = 0;
	xv_set(e->duration, PANEL_VALUE, e->durval, 0);
	xv_set(e->time, PANEL_VALUE, e->timeval, 0);
	xv_set(e->ampm, PANEL_VALUE, e->ampmval, 0);
	xv_set(e->minhr, PANEL_VALUE, e->minhrval, 0);
}
extern void
set_default_what(e)
	Editor *e;
{
	int i=0;

	for(i=0; i < 5; i++) {
		sprintf(e->what[i].itemval, "");
		xv_set(e->what[i].item, PANEL_VALUE, e->what[i].itemval, 0);
	}
	xv_set(e->panel, PANEL_CARET_ITEM, e->what[0].item, 0); 
}
extern void
set_default_privacy(e)
        Editor *e;
{
        Props *p = (Props*)calendar->properties;
        xv_set(e->privacyunit, PANEL_LABEL_STRING, privacystr[(int)p->privacyunit_VAL], NULL);
	e->privacyval = p->privacyunit_VAL;
}

extern void
set_defaults(e)
	Editor *e;
{
	Props *p = (Props*)calendar->properties;

	if (!editor_exists(e)) 
		make_editor(calendar);

        set_date_on_panel(calendar->view->date, e->datetext, 
			p->ordering_VAL, p->separator_VAL);      
	set_default_time(e);
	set_default_scope_plus(e);
	set_default_reminders(calendar);
	set_default_what(e);
	set_default_privacy(e);
	list_deselect_all(e->apptbox);

	datefilled = startfilled = true;
	whatfilled = false;
	set_dnd_target();
}


static void
decompose_duration(c, a)
	Calendar *c;  Appt *a;
{
	int i, r, ampm=0, hr, mn;	
	char buf[15];
	Browser *b = (Browser*)c->browser;
	Editor *d = (Editor*)c->editor;
	Props *p = (Props*)c->properties;

	if(d==NULL || a==NULL) return;

	i = a->duration;
	if (i == 0) {
		xv_set(d->duration, PANEL_VALUE, "", 0);
	        xv_set(d->minhr, PANEL_VALUE, ampm, 0);
		return;
	}
	r = i+a->appt_id.tick;
	hr = hour(r);
	mn = minute(r);
	if (p->default_disp_VAL == hour12) {
		if (!adjust_hour(&hr)) 
			ampm=1;
		sprintf(buf, "%2d:%02d", hr, mn);
		xv_set(d->minhr, PANEL_VALUE, ampm, 0);
	}
	else 
		sprintf(buf, "%02d%02d", hr, mn);

	xv_set(d->duration, PANEL_VALUE, buf, 0);
}

static Boolean
valid_time(time_str)
char *time_str;
{
	char 	*ptr;
	register int num_minutes=0, num_colons=0;
	Props *p = (Props*)calendar->properties;
	Boolean after_colon = FALSE;

	for (ptr = time_str; ptr != NULL && *ptr != '\0'; ptr++) {
		if (p->default_disp_VAL == hour12) {
			if (*ptr == ':') {
				after_colon = TRUE;
                      		if ((++num_colons) > 1)
                                	return false;
				if (*(ptr+1) == '\0')
					return false;
			}
			else if (*ptr != ' ' && (*ptr < '0' || *ptr > '9') )
				return false;
			if ((after_colon) && (*ptr != ':'))
				num_minutes++;
			if (num_minutes > 2)
				return false;
		}
		else if (p->default_disp_VAL == hour24) {
			if (*ptr != ' ' && (*ptr < '0' || *ptr > '9') )
				return false;
			if (++num_minutes > 4)
				return false;
		}
			
	}
        if (p->default_disp_VAL == hour12 && ((int)atoi(time_str) > 12) )
                return false;
        else if (p->default_disp_VAL == hour24 && ((int)atoi(time_str) > 2359) )
                return false;

	return true;
}

static int
seconds_to_units(item, secs, unit)
	int item;
	int secs;
	int *unit;
{
	int unit_val=0, rem=0;
 
	if (strcmp(unitstr[item], MGET("Mins")) == 0) {
                unit_val = seconds_to_minutes(secs);
		*unit = minsunit;
	}
        else if (strcmp(unitstr[item], MGET("Hrs")) == 0) {
                unit_val = seconds_to_hours(secs, &rem);
		if (unit_val == 0 || rem != 0) {
                	unit_val = seconds_to_minutes(secs);
			*unit = minsunit; 
		}
		else 
			*unit = hrsunit; 
	}
        else if (strcmp(unitstr[item], MGET("Days")) == 0) {
                unit_val = seconds_to_days(secs, &rem);
		if (unit_val == 0 || rem != 0) {
			unit_val = seconds_to_hours(secs, &rem);
                	if (unit_val == 0 || rem != 0) {
                       		unit_val = seconds_to_minutes(secs);
                        	*unit = minsunit;  
			}
			else 
				*unit = hrsunit;
                } 
		else
			*unit = daysunit; 
	}
	else {
                unit_val = seconds_to_minutes(secs);
		*unit = minsunit;
	}
 
        return unit_val;
}

static Advunit
units_to_val(item)
	Panel_item item;
{
	char *unit_str;
	Advunit val;

	unit_str = (char*)xv_get(item, PANEL_LABEL_STRING);

	if (strcmp(unit_str, MGET("Mins")) == 0)
		val = minsunit;
	else if (strcmp(unit_str, MGET("Hrs")) == 0)
		val = hrsunit;
	else if (strcmp(unit_str, MGET("Days")) == 0)
		val = daysunit;
	else
		val = minsunit;

	return val;
}
extern int
units_to_secs(unit, adv)
	Advunit unit;
	char *adv;
{
	int secs=0;

	switch(unit) {
		case minsunit: 
			secs = minutes_to_seconds( atoi(adv));
			break;
		case hrsunit:
			secs = hours_to_seconds( atoi(adv));
			break; 
		case daysunit:
			secs = days_to_seconds( atoi(adv));
			break; 
	}

	return secs;
}
/* first search for entire string, then search on user names */
extern Boolean
name_in_list(c, name, list, nameinlist)
	Calendar *c;
	char *name, *list, **nameinlist;
{
	char *listname, *uname, *ulistname, *l;

	*nameinlist = NULL;
	if (name == NULL || *name == NULL ||
		list == NULL || *list == NULL) 
		return false;
	
	l = cm_strdup(list);
	listname = (char*)strtok(l, " ");
	while (listname != NULL) {
		if (strcmp(name, listname) == 0) {
			*nameinlist = cm_strdup(listname);
			free(l);
			return true;
		}
		listname = (char*)strtok(NULL, " ");
	}
	free(l);
	uname = cm_target2name(name);
	l = cm_strdup(list);
	listname = (char*)strtok(l, " ");
	while (listname != NULL) {
		ulistname = cm_target2name(listname);
                if (strcmp(uname, ulistname) == 0) {
			*nameinlist = cm_strdup(listname);
                        free(ulistname); free(uname); free(l);
                        return true;
                }
		listname = (char*)strtok(NULL, " ");
		free(ulistname);
	}
	free(uname); free(l);
	*nameinlist = NULL;
	return false;
	
}

static Attribute*
mailto2appt(c, name)
	Calendar *c;
	char *name;
{
	char *mname=NULL;
	Attribute *attr=NULL;
	Editor *e = (Editor*)c->editor;
	Props *p = (Props*)c->properties;
	char buf[35], *adv=NULL;

	if (list_num_selected(e->calbox) <= 1) {
		attr = make_attr();	
		attr->attr = cm_strdup("ml");
		adv = (char*)xv_get(e->mailadvance, PANEL_VALUE);
		sprintf(buf, "%d", 
			units_to_secs(units_to_val(e->mailunit), adv));
		attr->value = cm_strdup(buf);
		if ((mname = (char*)xv_get(e->to, PANEL_VALUE)) == NULL || 
			*mname == NULL)
			attr->clientdata = cm_strdup(p->mailto_VAL);
		else
			attr->clientdata = cm_strdup(mname);
		attr->next = NULL;
		return attr;
	} 
	if (name_in_list(c, name, xv_get(e->to, PANEL_VALUE), 
				&mname)) {
		attr = make_attr();	
		attr->attr = cm_strdup("ml");
		adv = (char*)xv_get(e->mailadvance, PANEL_VALUE);
		sprintf(buf, "%d", 
			units_to_secs(units_to_val(e->mailunit), adv));
		attr->value = cm_strdup(buf);
		attr->clientdata = mname;
		attr->next = NULL;
	}
	return attr;
}
static char *
e_get_mailname(c, name)
	Calendar *c;
	char *name;
{
	Editor *e = (Editor*)c->editor;
	Props *p = (Props*)c->properties;
	char *mname = NULL;
	int n;

	/* if user's calendar is only one in calendar box, then just
	use the default mailto value. If > 1 name in calenadar box and
	is the user calendar, get name from default mail list. If
	not user calendar just return the calendar name */
	if (strcmp(name, c->calname) != 0) return name;
	if ((n = xv_get(e->calbox, PANEL_LIST_NROWS)) <= 1) {
		if (strcmp(name, c->calname) == 0)
			return p->mailto_VAL;
		return name;
	}
	if (strcmp(name, c->calname) == 0) {
		name_in_list(c, name, p->mailto_VAL, &mname);
		if (mname != NULL)
			return mname;
	}
	return name;
}
static void
update_mailto(c)
	Calendar *c;
{
	Editor *e = (Editor*)c->editor;
	Props *p = (Props*)c->properties;
	int i = -1;
	char buf[BUFSIZ+1], *name;
	
	*buf = 0;
	i = xv_get(e->calbox, PANEL_LIST_FIRST_SELECTED);
	while (i != -1) {
		name = (char*)xv_get(e->calbox, PANEL_LIST_CLIENT_DATA, i);
		if (name != NULL && *name != NULL)
			cm_strcat(buf, e_get_mailname(c, name));
		i = xv_get(e->calbox, PANEL_LIST_NEXT_SELECTED, i);
		if (i != -1)
			cm_strcat(buf, " ");
	}
	xv_set(e->to, PANEL_VALUE, buf, NULL); 
}

/*  called when 'insert' 'change' button is hit to consume panel values and
    stuff into an appointment	*/
static Appt * 
panel_to_appt(c)
	Calendar *c;
{
	char buf[35];
	Appt	*a;
	Browser	*b;
	Props	*p;
	Editor	*d;
	char	*adv=NULL;
	Attribute *attr;
	char	*s1=NULL, *s2=NULL, *s3=NULL, *s4=NULL;
	char	*startval=NULL, *endval = NULL, *date = NULL;
	int	secs=0, l=0, next=0, alert_res=0;
	Boolean	showtime = true;


	/* Start and end times are checked for validity. If they both
		exist and are valid then everything is just honky-dory. 
		If start and end times exist but either is invalid, 
		an error message is generated and routine returns.
		If start time exists and is valid and end time doesn't
		exist, then the end time defaults to start time plus one
		hour. If start time doesn't exist but end time exists,
		then its a error. If neither start nor end times exist,
		then the start time is set to 3:41 am and the end 
		time to 3:41 (magic time) plus one minute.
		Got it??
	*/

	if (c==NULL) return(NULL);
	b = (Browser *) c->browser;
	d = (Editor *) c->editor;
	p = (Props *) c->properties;

	/* Start time */
	startval = (char *) xv_get(d->time, PANEL_VALUE);
	/* End time */
	endval  = (char *) xv_get(d->duration, PANEL_VALUE);
	if ( (date = (char *) get_date_str(p, d->datetext) ) == NULL ) {
		xv_set(d->frame, FRAME_LEFT_FOOTER, EGET("Error in Date Field..."), NULL);
		return (NULL);
	}

	if (startval != NULL && *startval != '\0') {
		if (valid_time(startval)) {
			(void) sprintf(buf, "%s %s", date, startval); 
			if (p->default_disp_VAL == hour12)
				(void)cm_strcat(buf, 
					(xv_get(d->ampm, PANEL_VALUE)? "pm":"am"));
		}
		else { 
			xv_set(d->frame, FRAME_LEFT_FOOTER,
                                 EGET("Error in START Time...") , 0);
                        return(NULL);
                }
		if (endval != NULL && *endval != '\0') {
			if (!valid_time(endval)) {
                		xv_set(d->frame, FRAME_LEFT_FOOTER,
                        		 EGET("Error in STOP Time...") , 0);
                		return(NULL);
			}
		}
	}
	else if (endval != NULL && *endval != '\0') {
	/* if start time is null but there is an end time: error */
		xv_set(d->frame, FRAME_LEFT_FOOTER,
			 EGET("Specify a START Time...") , 0);
                        return(NULL);
	}
	else {
	/* if start time and end times are null this is a 'to-do' item */
		showtime = false;
		(void) sprintf(buf, "%s 3:41 am", date);
	}

	/* create appt, build up date/time string from panel, send it 
		through parser to get a tick.		 */

	a = make_appt();

	if (xv_get(d->appt_type, PANEL_VALUE))
		a->tag->tag = toDo;
	a->privacy = d->privacyval;

	a->appt_id.tick = (int) cm_getdate(buf, NULL);
	if(a->appt_id.tick < 0) {
		destroy_appt(a);
		xv_set(d->frame, FRAME_LEFT_FOOTER,
			 EGET("Error in Date Field...") , 0);
		return(NULL);
	}

	/* stuff in what strings */

	/* optimize this sometime with a loop */
        s1 = (char *) xv_get(d->what[0].item, PANEL_VALUE);
	s2 = (char *) xv_get(d->what[1].item, PANEL_VALUE);
	s3 = (char *) xv_get(d->what[2].item, PANEL_VALUE);
	s4 = (char *) xv_get(d->what[3].item, PANEL_VALUE);

	l = cm_strlen(s1) + cm_strlen(s2) + cm_strlen(s3) + 
		cm_strlen(s4);
	a->what = (char *) ckalloc(l + 10);
	if (!blank_buf(s1)) {
		(void) cm_strcat(a->what, s1);
		(void) cm_strcat(a->what, "\n");
	}
	if (!blank_buf(s2)) {
		(void) cm_strcat(a->what, s2);
		(void) cm_strcat(a->what, "\n");
	}
	if (!blank_buf(s3)) {
		(void) cm_strcat(a->what, s3);
		(void) cm_strcat(a->what, "\n");
	}
	if (!blank_buf(s4)) {
		(void) cm_strcat(a->what, s4);
		(void) cm_strcat(a->what, "\n");
	}
	/* can't have null appt. */
	if (a->what[0] == '\0') {
                a->what[0] = ' ';
                a->what[1] = '\0';
        }
	/* don't allow an appt that has an empty What string with No time */
        if (blank_buf(a->what) && !showtime) {
		notice_prompt(d->frame, (Event *)NULL,
                    NOTICE_MESSAGE_STRINGS,
                    EGET("Bad Appointment: No START, STOP, and WHAT specified."),
                    0,
                    NOTICE_BUTTON_YES,  LGET("Continue") ,
                    NULL);
                destroy_appt(a);
                return((Appt*)NULL);
        }

	/* Period Exclusive Choice */

	a->period.period = d->periodval;
	a->period.nth = d->nthval;
	if (a->period.period != single) {
		if (strcmp((char*)xv_get(d->scope, PANEL_VALUE),  
					LGET("forever") )==0) 
			a->ntimes=CMFOREVER;
		else {
                        if ((a->ntimes = atoi((char*)xv_get(d->scope, PANEL_VALUE))) 
                                                == 0) {           
				xv_set(d->frame, FRAME_LEFT_FOOTER,
                                         EGET("Error in FOR Field...") , 0);
                                destroy_appt(a);
                                return((Appt*)NULL);
                        }
                }
	}

	/* Duration: */
	if (endval != NULL && *endval != '\0') {
		(void) sprintf(buf, "%s %s", date,  endval);
		if (p->default_disp_VAL == hour12)
			(void)cm_strcat(buf, (xv_get(d->minhr, PANEL_VALUE)?
					"pm":"am"));
		next = (int) cm_getdate(buf, NULL);
		if (next < a->appt_id.tick) {
			alert_res = notice_prompt(d->frame, (Event *)NULL,
			NOTICE_MESSAGE_STRINGS,
				 MGET("This appointment has an end\ntime earlier than its begin\ntime.  Do you want to schedule\nit into the next day?") ,
			0,
			NOTICE_BUTTON_YES,  LGET("Yes") ,
			NOTICE_BUTTON_NO,   LGET("Cancel") ,
			0);
			switch(alert_res) {
				case NOTICE_YES:
					a->duration = (next + daysec) - a->appt_id.tick;
					break;
				case NOTICE_NO:
					destroy_appt(a); 
					return((Appt*)NULL);
			}
		}
		else {
			a->duration = next - a->appt_id.tick;
		}
	}
	/* if start and end times are NULL: minute dur */
	else if (!showtime)  {
		a->duration = (long)minsec;
		a->tag->showtime = false;
	}
	else /* NULL end time with valid start time */ 
		a->duration = 0;
	/* Reminder Non-exclusive Choice */
	if (beep_on(d)) {
		attr = make_attr();
		attr->attr = cm_strdup("bp");
		adv = (char*)xv_get(d->beepadvance, PANEL_VALUE);
		d->beepunitval = units_to_val(d->beepunit);
		secs = units_to_secs(d->beepunitval, adv);
		sprintf(buf, "%d", secs);
		attr->value = cm_strdup(buf);
		attr->next = a->attr;
		a->attr = attr;
	}
	if (flash_on(d)) {
		attr = make_attr();
		attr->attr = cm_strdup("fl");
		adv = (char*)xv_get(d->flashadvance, PANEL_VALUE);
		d->flashunitval = units_to_val(d->flashunit);
		secs = units_to_secs(d->flashunitval, adv);
		sprintf(buf, "%d", secs);
		attr->value = cm_strdup(buf);
		attr->next = a->attr;
		a->attr = attr;
	}
	if (open_on(d)) {
		attr = make_attr();
		attr->attr = cm_strdup("op");
		adv = (char*)xv_get(d->openadvance, PANEL_VALUE);
		d->openunitval = units_to_val(d->openunit);
		secs = units_to_secs(d->openunitval, adv);
		sprintf(buf, "%d", secs);
		attr->value = cm_strdup(buf);
		attr->next = a->attr;
		a->attr = attr;
	}
	xv_set(d->frame, FRAME_LEFT_FOOTER, "", 0);
	return(a);
}
		  
/* ARGSUSED */
static void
calbox_notify(pi, string, cd, op, event, row)
	Panel_item pi;
	char *string;       /* string of row being operated on */
    	Xv_opaque cd;  /* Client data of row */
   	Panel_list_op op;   /* operation */
    	Event *event;
   	int row;		
{
	Boolean in_list = false;
	char *name, *tmp;
	Calendar *c = (Calendar*)xv_get(pi, PANEL_CLIENT_DATA);
	Editor *e = (Editor*)c->editor;

	name = (char*)xv_get(pi, PANEL_LIST_CLIENT_DATA, row);
	in_list = name_in_list(c, name, xv_get(e->to, PANEL_VALUE), &tmp);
	if (tmp != NULL) free(tmp);
	if ((op == PANEL_LIST_OP_SELECT && !in_list) ||
		(op == PANEL_LIST_OP_DESELECT && in_list))
		update_mailto(c);
}
/* ARGSUSED */
static void
apptbox_notify(pi, string, cd, op, event, row)
	Panel_item pi;
	char *string;       /* string of row being operated on */
    	Xv_opaque cd;  /* Client data of row */
   	Panel_list_op op;   /* operation */
    	Event *event;
   	int row;		
	
{
        Calendar *c;
        Editor *e;
        Browser *b;

	c = (Calendar *) xv_get(pi, PANEL_CLIENT_DATA);
	e = (Editor *) c->editor;
	b = (Browser *) c->browser;

	if (op == PANEL_LIST_OP_SELECT) {
		list_cd_flush(e->calbox);
		apptbox_action(c, row);
	}
	else if (op == PANEL_LIST_OP_DESELECT) {
		if (xv_get(pi, PANEL_LIST_FIRST_SELECTED) == -1)
			if (browser_showing(b))
				browser2calbox(c);
			else 
				set_default_calbox(c);	
		xv_set(e->frame, FRAME_LEFT_FOOTER, "", 0);
	}
}

static char *minitem = "00";

/* ARGSUSED */
static void
set_start_hour(m, mi)
	Menu m;
	Menu_item mi;
{
	int t=0, t1=0, h=0, mn=0;
	char *str;
	char buf[35];
	Browser *b;
	Editor *e;
	Props *p;

	b = (Browser *) calendar->browser;
	e = (Editor *) calendar->editor;
	p = (Props *) calendar->properties;

	t = (int) xv_get(mi, MENU_CLIENT_DATA);

	if (p->default_disp_VAL == hour12)
		if (t > 11)
			xv_set(e->ampm, PANEL_VALUE, 1, 0);
		else
			xv_set(e->ampm, PANEL_VALUE, 0, 0);

	str = (char *) xv_get(mi, MENU_STRING, NULL);

	if (t == -1)
		startfilled = false;
	else
		startfilled = true;
	set_dnd_target();

	if (t == -1) {
                xv_set(e->time, PANEL_VALUE, "", 0);
                xv_set(e->duration, PANEL_VALUE, "", 0);
                xv_set(e->minhr, PANEL_VALUE, 0, 0);
        }
        else if (t == -2) {
		if (p->default_disp_VAL == hour12) {
                	xv_set(e->time, PANEL_VALUE, "12:00", 0);
                	xv_set(e->duration, PANEL_VALUE, "11:59", 0);
                	xv_set(e->minhr, PANEL_VALUE, 1, 0);
		}
		else {
                	xv_set(e->time, PANEL_VALUE, "0000", 0);
                	xv_set(e->duration, PANEL_VALUE, "2359", 0);
		}
        }
	else {
		cm_strcpy(buf, str);
                cm_strcat(buf, minitem);
                xv_set(e->time, PANEL_VALUE, buf, 0);
        }


	if (t != -1 && t != -2) { /* Not None or All Day*/
		if (p->default_disp_VAL == hour12) {
			if ((t+1) > 11)
				xv_set(e->minhr, PANEL_VALUE, 1, NULL);
			else 
				xv_set(e->minhr, PANEL_VALUE, 0, NULL);
			if ( t == 23  &&  (xv_get(e->ampm, PANEL_VALUE) == 1) )
				/* Start time = 11pm so end time must set to 12am */
				xv_set(e->minhr, PANEL_VALUE, 0, NULL);
		}
		(void) cm_strcpy(buf, (char*)get_date_str(p, e->datetext));
		(void) cm_strcat(buf, " ");
		(void) cm_strcat(buf, (char *)xv_get(e->time, PANEL_VALUE));
		if (p->default_disp_VAL == hour12)
			(void) cm_strcat(buf, xv_get(e->ampm, PANEL_VALUE)?
					"pm":"am" );
		t = (int) cm_getdate(buf, NULL);
		t1 = t + hrsec;
		t1 = adjust_dst(t, t1);
		h = hour(t1);
		mn = minute(t1);
		if (p->default_disp_VAL == hour12) {
			adjust_hour(&h);
			sprintf(buf, "%2d:%02d", h, mn);
		}
		else
			sprintf(buf, "%02d%02d", h, mn);

		xv_set(e->duration, PANEL_VALUE, buf, 0);
	}
}

/* ARGSUSED */
static void
set_end_hour(m, mi)
	Menu m;
	Menu_item mi;
{
	int t;
	char *str;
	char buf[10];
	Browser *b;
	Editor *e;
	Props *p;

	b = (Browser *) calendar->browser;
	e = (Editor *) calendar->editor;
	p = (Props *) calendar->properties;

	t = (int) xv_get(mi, MENU_CLIENT_DATA);
	if (p->default_disp_VAL == hour12) 
		if (t > 11)
			xv_set(e->minhr, PANEL_VALUE, 1, 0);
		else
			xv_set(e->minhr, PANEL_VALUE, 0, 0);

	str = (char *) xv_get(mi, MENU_STRING, NULL);

	if (t != -1) {  /* None */
		cm_strcpy(buf, str);
		cm_strcat(buf, minitem);
	}
	else 
		buf[0] = '\0';

	xv_set(e->duration, PANEL_VALUE, buf, 0);
}

/* ARGSUSED */
static void
set_min(m, mi)
	Menu m;
	Menu_item mi;
{
        Menu_item time_item;
	Menu parent_menu=NULL;

	minitem = (char *) xv_get(mi, MENU_STRING, NULL);
        time_item = (Menu_item)xv_get(m, MENU_PARENT, NULL);
	set_start_hour (parent_menu, time_item);
        minitem = "00";
        
}

/* ARGSUSED */
static void
set_scope(m, mi)
	Menu m;
	Menu_item mi;
{
	Editor *e;

	e = (Editor *) xv_get(m, MENU_CLIENT_DATA);
	xv_set(e->scope, PANEL_VALUE,
		(char *) xv_get(mi, MENU_STRING, NULL), 0);

}
	
static Menu
mb_make_scope_menu(e)
	Editor *e;
{
	Menu scopemenu;

	if (e==NULL) return(NULL);
	scopemenu = menu_create(
		/* MENU_NOTIFY_PROC, set_scope, */
		MENU_CLIENT_DATA, e,
		MENU_ITEM,
			MENU_STRING, "2",
			MENU_VALUE, 2,
			MENU_ACTION_PROC, set_scope,
			0,
		MENU_ITEM,
			MENU_STRING, "3",
			MENU_VALUE, 3,
			MENU_ACTION_PROC, set_scope,
			0,
		MENU_ITEM,
			MENU_STRING, "4",
			MENU_VALUE, 4,
			MENU_ACTION_PROC, set_scope,
			0,
		MENU_ITEM,
			MENU_STRING, "5",
			MENU_VALUE, 5,
			MENU_ACTION_PROC, set_scope,
			0,
		MENU_ITEM,
			MENU_STRING, "6",
			MENU_VALUE, 3,
			MENU_ACTION_PROC, set_scope,
			0,
		MENU_ITEM,
			MENU_STRING, "7",
			MENU_VALUE, 7,
			MENU_ACTION_PROC, set_scope,
			0,
		MENU_ITEM,
			MENU_STRING, "8",
			MENU_VALUE, 8,
			MENU_ACTION_PROC, set_scope,
			0,
		MENU_ITEM,
			MENU_STRING, "9",
			MENU_VALUE, 9,
			MENU_ACTION_PROC, set_scope,
			0,
		MENU_ITEM,
			MENU_STRING, "10",
			MENU_VALUE, 10,
			MENU_ACTION_PROC, set_scope,
			0,
		MENU_ITEM,
			MENU_STRING, "11",
			MENU_VALUE, 11,
			MENU_ACTION_PROC, set_scope,
			0,
		MENU_ITEM,
			MENU_STRING, "12",
			MENU_VALUE, 12,
			MENU_ACTION_PROC, set_scope,
			0,
		MENU_ITEM,
			MENU_STRING, "13",
			MENU_VALUE, 13,
			MENU_ACTION_PROC, set_scope,
			0,
		MENU_ITEM,
			MENU_STRING, "14",
			MENU_VALUE, 14,
			MENU_ACTION_PROC, set_scope,
			0,
		MENU_ITEM,
			MENU_STRING,  LGET("forever") ,
			MENU_VALUE, CMFOREVER,
			MENU_ACTION_PROC, set_scope,
			0,
		XV_HELP_DATA, "cm:RepeatTimes",
		0);
	return(scopemenu);
}

static char *duritem = "00";

/* ARGSUSED */
static void
set_dur(m, mi)
	Menu m;
	Menu_item mi;
{
        Menu_item time_item;
	Menu parent_menu=NULL;
        static void set_duration();

	duritem = (char *) xv_get(mi, MENU_STRING, NULL);
        time_item = (Menu_item)xv_get(m, MENU_PARENT);
	set_duration (parent_menu, time_item);
        duritem = "00";
}

/* ARGSUSED */
static void
set_duration(m, mi)
	Menu m;
	Menu_item mi;
{
	int t;
        char *str;
        char buf[10];
        Browser *b;
        Editor *e;
        Props *p;

        b = (Browser *) calendar->browser;
        e = (Editor *) calendar->editor;
	p = (Props *) calendar->properties;

        str = (char *) xv_get(mi, MENU_STRING, NULL);
        cm_strcpy(buf, str);
        cm_strcat(buf, duritem);
        xv_set(e->duration, PANEL_VALUE, buf, 0);
        t = (int) xv_get(mi, MENU_CLIENT_DATA);
	if (p->default_disp_VAL == hour12)
        	if (t > 11)
                	xv_set(e->minhr, PANEL_VALUE, 1, 0);
        	else      
                	xv_set(e->minhr, PANEL_VALUE, 0, 0);
}

extern Menu
e_make_duration_menu(e)
	Editor *e;
{
	int i=0, j=0;
	char buf[10];
	Props *p=NULL;
	Menu timemenu;
	Menu minutemenu;
	Menu_item item;
	
	if (e==NULL) return(NULL);

	p = (Props *) calendar->properties;
	timemenu = (Menu) xv_get(e->durationstack, PANEL_ITEM_MENU);
	if (timemenu != NULL) {
	/* space leak: child menu left dangling */
		menu_destroy(timemenu);
	}

	minutemenu = menu_create(
		MENU_CLIENT_DATA, calendar,
		MENU_NOTIFY_PROC, set_dur,
		MENU_ITEM, 
			MENU_STRING, "00",
			MENU_VALUE, 0,
			MENU_ACTION_PROC, set_dur,
			0,
		MENU_ITEM, 
			MENU_STRING, "15",
			MENU_VALUE, 15,
			MENU_ACTION_PROC, set_dur,
			0,
		MENU_ITEM, 
			MENU_STRING, "30",
			MENU_VALUE, 30,
			MENU_ACTION_PROC, set_dur,
			0,
		MENU_ITEM, 
			MENU_STRING, "45",
			MENU_VALUE, 45,
			MENU_ACTION_PROC, set_dur,
			0,
		XV_HELP_DATA, "cm:ApptDuration",
		0);
		
	timemenu = menu_create(
		MENU_CLIENT_DATA, calendar,
		MENU_NOTIFY_PROC, set_duration,
		MENU_NOTIFY_STATUS, XV_ERROR,
		XV_HELP_DATA, "cm:ApptDuration",
		0);

	i = p->begin_slider_VAL;
	j = p->end_slider_VAL;

	for(; i < j; i++) {
		if (p->default_disp_VAL == hour12) {
			if (i > 12)
                       		 sprintf(buf, "%2d:", i-12);
                	else
                       		 if (i == 0)
                               		 cm_strcpy(buf, "12:");
                	else
                       		 sprintf(buf, "%2d:", i);
		}
		else
			sprintf(buf, "%02d", i);
		item = menu_create_item(MENU_PULLRIGHT_ITEM,
				cm_strdup(buf), minutemenu,
				MENU_ACTION_PROC, set_end_hour,
				0);
		menu_set(item, MENU_CLIENT_DATA, i, 0);
		menu_set(timemenu,
				MENU_APPEND_ITEM, item,
				0);
	}
	item = menu_create_item(MENU_STRING, NONE,
					MENU_CLIENT_DATA,  -1,
					MENU_ACTION_PROC, set_end_hour,
					NULL);
	menu_set(timemenu,
			MENU_APPEND_ITEM, item,
		NULL);

	return(timemenu);
}
static void
mb_set_period(m, mi)
	Menu m;
	Menu_item mi;
{
	Editor *e;

	e = (Editor*)xv_get(m, MENU_CLIENT_DATA); 
	e->periodval = (int)xv_get(mi, MENU_VALUE);
	e->nthval = 0;
	xv_set(e->periodunit, PANEL_LABEL_STRING, 
		(char*)xv_get(mi, MENU_STRING), NULL);
	activate_scope(e);
	xv_set(e->scope, PANEL_VALUE, repeatval[(int)e->periodval], 0);
	xv_set(e->scopeunit, PANEL_LABEL_STRING, repeatstr[(int)e->periodval], 0);
	
}
static void
mb_set_priv(m, mi)
	Menu m;
	Menu_item mi;
{
	Editor *e;

	e = (Editor*)xv_get(m, MENU_CLIENT_DATA); 
	e->privacyval = (int)xv_get(mi, MENU_VALUE);
	xv_set(e->privacyunit, PANEL_LABEL_STRING, 
		(char*)xv_get(mi, MENU_STRING), NULL);
}
static Menu
mb_make_priv_menu(e)
        Editor *e;
{
        static Menu pmenu;

        if (e==NULL) return(NULL);

        pmenu = menu_create(
                MENU_CLIENT_DATA, e,
                MENU_NOTIFY_PROC, mb_set_priv,
                MENU_NOTIFY_STATUS, XV_ERROR,
                MENU_ITEM,
                        MENU_STRING,  privacystr[0],
                        MENU_VALUE, 0,
                        0,
                MENU_ITEM,
                        MENU_STRING,  privacystr[2],
                        MENU_VALUE, 2,
                        0,
                MENU_ITEM,
                        MENU_STRING,  privacystr[1],
                        MENU_VALUE, 1,
                        0,
                XV_HELP_DATA, "cm:PrivacyMenu",
                0);

        return(pmenu);
}
static Menu
mb_make_period_menu(e)
        Editor *e;
{
        static Menu pmenu;

        if (e==NULL) return(NULL);

        pmenu = menu_create(
                MENU_CLIENT_DATA, e,
                MENU_NOTIFY_STATUS, XV_ERROR,
                MENU_ITEM,
                        MENU_STRING,  periodstr[single] ,
                        MENU_VALUE, 0,
                	MENU_NOTIFY_PROC, mb_set_period,
                        0,
                MENU_ITEM,
                        MENU_STRING,  periodstr[daily],
                        MENU_VALUE, 1,
                	MENU_NOTIFY_PROC, mb_set_period,
                        0,
                MENU_ITEM,
                        MENU_STRING,  periodstr[weekly],
                        MENU_VALUE, 2,
                	MENU_NOTIFY_PROC, mb_set_period,
                        0,
                MENU_ITEM,
                        MENU_STRING,  periodstr[biweekly],
                        MENU_VALUE, 3,
                	MENU_NOTIFY_PROC, mb_set_period,
                        0,
                MENU_ITEM,
                        MENU_STRING,  periodstr[monthly],
                        MENU_VALUE, 4,
                	MENU_NOTIFY_PROC, mb_set_period,
                        0,
                MENU_ITEM,
                        MENU_STRING,  periodstr[nthWeekday],
                        MENU_VALUE, 6,
                	MENU_NOTIFY_PROC, mb_set_period,
                        0,
                MENU_ITEM,
                        MENU_STRING,  periodstr[yearly],
                        MENU_VALUE, 5,
                	MENU_NOTIFY_PROC, mb_set_period,
                        0,
                MENU_ITEM,
                        MENU_STRING,  periodstr[monThruFri],
                        MENU_VALUE, 11,
                	MENU_NOTIFY_PROC, mb_set_period,
                        0,
                MENU_ITEM,
                        MENU_STRING,  periodstr[monWedFri],
                        MENU_VALUE, 12,
                	MENU_NOTIFY_PROC, mb_set_period,
                        0,
                MENU_ITEM,
                        MENU_STRING,  periodstr[tueThur],
                        MENU_VALUE, 13,
                	MENU_NOTIFY_PROC, mb_set_period,
                        0,
                MENU_ITEM,
                        MENU_STRING,  LGET("Repeat Every...") ,
                        MENU_VALUE, 7,
			MENU_ACTION_PROC, show_repeat, 
                        0,
                XV_HELP_DATA, "cm:RepeatUnit",
                0);

        return(pmenu);
}
 
	
extern Menu
e_make_time_menu(e)
	Editor *e;
{
	int i=0, j=0;
	char buf[10];
	Props *p=NULL;
	Menu minutemenu;
        Menu timemenu;
	Menu_item item;
	
	if (e==NULL) return(NULL);

	p = (Props *) calendar->properties;
	timemenu = (Menu) xv_get(e->timestack, PANEL_ITEM_MENU);
	if (timemenu != NULL) {
	/* space leak: child menu left dangling */
		menu_destroy(timemenu);
	}

	minutemenu = menu_create(
		MENU_CLIENT_DATA, calendar,
		MENU_NOTIFY_PROC, set_min,
		MENU_ITEM, 
			MENU_STRING, "00",
			MENU_VALUE, 0,
			MENU_ACTION_PROC, set_min,
			0,
		MENU_ITEM, 
			MENU_STRING, "15",
			MENU_VALUE, 15,
			MENU_ACTION_PROC, set_min,
			0,
		MENU_ITEM, 
			MENU_STRING, "30",
			MENU_VALUE, 30,
			MENU_ACTION_PROC, set_min,
			0,
		MENU_ITEM, 
			MENU_STRING, "45",
			MENU_VALUE, 45,
			MENU_ACTION_PROC, set_min,
			0,
		XV_HELP_DATA, "cm:ApptTime",
		0);
		
	timemenu = menu_create(
		MENU_CLIENT_DATA, calendar,
		MENU_NOTIFY_PROC, set_start_hour,
		MENU_NOTIFY_STATUS, XV_ERROR,
		XV_HELP_DATA, "cm:ApptTime",
		0);

	i = p->begin_slider_VAL;
	j = p->end_slider_VAL;

	for(; i < j; i++) {
		if (p->default_disp_VAL == hour12) {
			if (i > 12)
                       		 sprintf(buf, "%2d:", i-12);
                	else
                        	if (i == 0)
                                	cm_strcpy(buf, "12:");
                	else
                       		 sprintf(buf, "%2d:", i);	
		}
		else
			sprintf(buf, "%02d", i);
		item = menu_create_item(MENU_PULLRIGHT_ITEM,
				cm_strdup(buf), minutemenu,
				MENU_ACTION_PROC, set_start_hour,
		0);
		menu_set(item, MENU_CLIENT_DATA, i, 0);
		menu_set(timemenu, MENU_APPEND_ITEM, item, 0);
	}
	item = menu_create_item( MENU_STRING,  NONE,
		MENU_CLIENT_DATA,  -1,
		MENU_ACTION_PROC, set_start_hour,
	NULL);
	menu_set(timemenu,
		MENU_APPEND_ITEM, item,
	NULL);

	item = menu_create_item( MENU_STRING,  LGET("All Day") ,
		MENU_CLIENT_DATA,  -2,
		MENU_ACTION_PROC, set_start_hour,
	NULL);
        menu_set(timemenu,
		MENU_APPEND_ITEM, item,
	NULL);

	return(timemenu);
}
extern void
e_hide_ampm(c)
        Calendar *c;
{
        Editor *e = (Editor*)c->editor;
        Props *p = (Props*)c->properties;
	int l;

	if (!editor_exists(e)) return;
        if (p->default_disp_VAL == hour24) {
		l = xv_get(e->datetext, PANEL_VALUE_DISPLAY_LENGTH);
                xv_set(e->ampm, XV_SHOW, FALSE, NULL);
                xv_set(e->minhr, XV_SHOW, FALSE, NULL);
                xv_set(e->time, PANEL_VALUE_DISPLAY_LENGTH, l-4, NULL);
                xv_set(e->duration, PANEL_VALUE_DISPLAY_LENGTH, l-4, NULL);
        }
        else {
                xv_set(e->time, PANEL_VALUE_DISPLAY_LENGTH, 6, NULL);
                xv_set(e->duration, PANEL_VALUE_DISPLAY_LENGTH, 6, NULL);
                xv_set(e->ampm, XV_SHOW, TRUE, NULL);
                xv_set(e->minhr, XV_SHOW, TRUE, NULL);
        }
}
static int
mail_on(d)
	Editor *d;
{
	int v = (int) xv_get(d->reminder, PANEL_VALUE);
	return(choice_on(v, MAIL));
}

static int
flash_on(d)
	Editor *d;
{
	int v = (int) xv_get(d->reminder, PANEL_VALUE);
	return(choice_on(v, FLASH));
}

static int
open_on(d)
	Editor *d;
{
	int v = (int) xv_get(d->reminder, PANEL_VALUE);
	return(choice_on(v, OPEN));
}

static int
beep_on(d)
	Editor *d;
{
	int v = (int) xv_get(d->reminder, PANEL_VALUE);
	return(choice_on(v, BEEP));
}


/* ARGSUSED */
static Panel_setting
validate_num(item, event)
        Panel_item      item;
        Event           *event;
{
        char    *input_str = (char*)xv_get(item, PANEL_VALUE);
        Editor   *e;
	Boolean  error = false;
	char save_str[80];

        /* event proc gets called for down and up event */
        if (event_is_down(event) 
		|| input_str == NULL || input_str[0] == '\0')
                return(panel_text_notify(item, event));

	cm_strcpy(save_str, input_str);
        while (input_str != NULL && input_str[0] != '\0') {
                if (!iscntrl(*input_str) && !isdigit(*input_str)) {
                	e = (Editor *) xv_get(item, PANEL_CLIENT_DATA);
                		notice_prompt(e->frame, (Event *)NULL,
                       		NOTICE_MESSAGE_STRINGS,  MGET("Invalid Input"),
                        	0,
                        	NOTICE_BUTTON_YES,  LGET("Continue") ,
                        0);
			/* overwrite input error */
			save_str[cm_strlen(save_str)-1] = '\0';
			xv_set(item, PANEL_VALUE, save_str, NULL); 
                	return(PANEL_NONE);
		}
		input_str++;
        }
        return(panel_text_notify(item, event));
}
static void
e_set_adv_unit(m, mi)
	Menu m;
	Menu_item mi;
{
	Panel_item pi;

	pi = (Panel_item)xv_get(m, MENU_CLIENT_DATA); 
	xv_set(pi, PANEL_LABEL_STRING, (char*)xv_get(mi, MENU_STRING), 
			NULL);
}

static Menu
e_make_unit_menu(pi)
	Panel_item pi;
{
        static Menu unitmenu;
	
	if (pi==NULL) return(NULL);

	unitmenu = menu_create(
		MENU_CLIENT_DATA, pi,
		MENU_NOTIFY_PROC, e_set_adv_unit,
		MENU_NOTIFY_STATUS, XV_ERROR,
		MENU_ITEM, 
			MENU_STRING,  MGET("Mins") ,
			MENU_VALUE, 0,
			0,
		MENU_ITEM, 
			MENU_STRING,  MGET("Hrs") ,
			MENU_VALUE, 1,
			0,
		MENU_ITEM, 
			MENU_STRING,  MGET("Days") ,
			MENU_VALUE, 2,
			0,
		XV_HELP_DATA, "cm:UnitMenu",
		0);
		
	return(unitmenu);
}

/* ARGSUSED */
extern Notify_value
date_notify(item, event)
	Panel_item item; Event *event;
{
	char *str = (char *)xv_get(item, PANEL_VALUE);

	if (str == NULL || cm_strlen(str) == 0)
		datefilled = false;
	else
		datefilled = true;

	set_dnd_target();

	return((Notify_value)panel_text_notify(item, event));
}

/* ARGSUSED */
extern Notify_value
start_notify(item, event)
	Panel_item item; Event *event;
{
	char *str = (char *)xv_get(item, PANEL_VALUE);

	if (str == NULL || cm_strlen(str) == 0)
		startfilled = false;
	else
		startfilled = true;

	set_dnd_target();

	return((Notify_value)panel_text_notify(item, event));
}

/* ARGSUSED */
extern Notify_value
what_notify(item, event)
	Panel_item item; Event *event;
{
	Editor *e = (Editor *)calendar->editor;
	char *str0 = (char *)xv_get(e->what[0].item, PANEL_VALUE);
	char *str1 = (char *)xv_get(e->what[1].item, PANEL_VALUE);
	char *str2 = (char *)xv_get(e->what[2].item, PANEL_VALUE);
	char *str3 = (char *)xv_get(e->what[3].item, PANEL_VALUE);

	if (str0 && cm_strlen(str0) > 0)
		whatfilled = true;
	else if (str1 && cm_strlen(str1) > 0)
		whatfilled = true;
	else if (str2 && cm_strlen(str2) > 0)
		whatfilled = true;
	else if (str3 && cm_strlen(str3) > 0)
		whatfilled = true;
	else
		whatfilled = false;

	set_dnd_target();

	return((Notify_value)panel_text_notify(item, event));
}

extern void
make_editor(c)
	Calendar *c;
{
	int x_gap, y_gap, x_row, i, wid;
	char *t, *msg;
	Font_string_dims dims;
	struct pr_size size1, size2, size3, size4, size5, size6;
	Xv_Font	pf;
	Rect *rect, *stack_rect;
	int old_win_row_gap, wd, wd_2, tmp = 0;
	Editor *e = (Editor*)c->editor;
	int tmp_x, stack_x;
	Browser *b = (Browser*)c->browser;
	Props *p = (Props*)c->properties;
	int col_setting, max_wid, minhr_wid, ampm_wid, alarm_wid, pfdef_wid, pvdl;
	Panel_item datestr, insert_button, delete_button, change_button, reset_button;
	Panel_item tmp_pi;
	char label[80];

#ifdef OW_I18N
	box_font = (Xv_Font) xv_find(calendar->frame, FONT,
                FONT_FAMILY,    FONT_FAMILY_DEFAULT_FIXEDWIDTH,
                FONT_SIZE,      12,
                0);
#else
	box_font = c->fonts->fixed12;
#endif
	pf = xv_get(c->frame, XV_FONT);
	pfdef_wid = (int)xv_get(pf, FONT_COLUMN_WIDTH);

	sprintf(label, "%s %s", LGET("CM Appointment Editor: "), c->calname);
	e->frame = xv_create(c->frame, FRAME_CMD,
		FRAME_SHOW_LABEL, TRUE,
		FRAME_CMD_PUSHPIN_IN, TRUE,
		FRAME_SHOW_FOOTER, TRUE,
		XV_LABEL,  label,
		WIN_USE_IM, TRUE,
		WIN_CLIENT_DATA, c,
		WIN_ROW_GAP, 9,
		FRAME_DONE_PROC, e_done_proc,
		0);
        e->panel = xv_get(e->frame, FRAME_CMD_PANEL);
	xv_set(e->panel,
		WIN_X, 0,
		WIN_Y, 0,
		WIN_CLIENT_DATA, c,
		WIN_ROW_GAP, 9,
		XV_HELP_DATA, "cm:EdPanel",
		0);


	t =  MGET("Date:") ;
	(void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size1.x = dims.width;
	size1.y = dims.height;
	t =  MGET("Start:") ;
	(void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size2.x = dims.width;
	size2.y = dims.height;
	t =  MGET("Stop:") ;
	(void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size3.x = dims.width;
	size3.y = dims.height;
	t =  MGET("What:") ;
	(void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size4.x = dims.width;
	size4.y = dims.height;
	t =  MGET("Alarm:") ;
	(void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size5.x = dims.width;
	size5.y = dims.height;
	t =  MGET("Mail To:") ;
	(void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size6.x = dims.width;
	size6.y = dims.height;

	wd = max(max(max(max(max(size1.x, size2.x), size3.x), size4.x), 
			size5.x), size6.x) + c->view->outside_margin;
	x_gap = 10;

	old_win_row_gap = xv_get(e->panel, WIN_ROW_GAP);
        xv_set(e->panel, WIN_ROW_GAP, 0, NULL);
	t =  MGET("Date:") ;
	datestr = xv_create(e->panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, t,
		PANEL_LABEL_BOLD, TRUE,
		XV_X, wd - size1.x,
		XV_Y, xv_row(e->panel, 1),
		XV_HELP_DATA, "cm:DateField",
		0);
	e->datetext = xv_create(e->panel, PANEL_TEXT,
		XV_SHOW, TRUE,
		PANEL_READ_ONLY, /* TRUE, */ FALSE,
		XV_X, (int)xv_get(datestr, XV_X) + size1.x + x_gap,
		XV_Y, xv_get(datestr, XV_Y),
		PANEL_NOTIFY_STRING, "\n\r\t",
		PANEL_NOTIFY_LEVEL, PANEL_ALL,
		PANEL_NOTIFY_PROC, date_notify,
		XV_HELP_DATA, "cm:DateField",
		0);
	tmp_x = xv_get(e->datetext, XV_X);
	msg = (char*)get_datemsg(p->ordering_VAL, p->separator_VAL);
	create_datelabel(e->panel, tmp_x, c->fonts->lucida9, 
			msg, &e->datefield);
	free(msg);
	xv_set(e->panel, WIN_ROW_GAP, old_win_row_gap, NULL);

	y_gap = 9;
	x_row = xv_get(e->datefield, XV_Y) + xv_get(e->datefield, XV_HEIGHT) + y_gap;
	t =  MGET("Start:") ;
	xv_create(e->panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, t,
		PANEL_LABEL_BOLD, TRUE,
		XV_X, wd - size2.x - 2,
		XV_Y, x_row,
		XV_HELP_DATA, "cm:ApptTime",
		0);
	e->timestack = xv_create(e->panel, PANEL_ABBREV_MENU_BUTTON,
		XV_X, tmp_x,
		XV_Y, x_row, 
		XV_HELP_DATA, "cm:ApptTime",
		0);

	xv_set(e->timestack, PANEL_ITEM_MENU, e_make_time_menu(e), 0);

	stack_rect = (Rect *) xv_get(e->timestack, PANEL_ITEM_RECT);
	e->time = xv_create(e->panel, PANEL_TEXT,
		PANEL_VALUE_DISPLAY_LENGTH, 6,
		PANEL_NOTIFY_PROC, start_notify,
		PANEL_NOTIFY_LEVEL, PANEL_ALL,
		XV_SHOW, TRUE,
		XV_X, xv_get(e->timestack, XV_X) + 
			stack_rect->r_width + x_gap,
		XV_Y, x_row,
		XV_HELP_DATA, "cm:ApptTime",
		0);
	rect = (Rect *) xv_get(e->time, PANEL_ITEM_RECT);
	e->ampm = xv_create(e->panel, PANEL_CHOICE,
		PANEL_CHOICE_STRINGS,  MGET(" AM ") ,  MGET(" PM ") , 0,
		XV_X, xv_get(e->time, XV_X) +
			rect->r_width + x_gap,
		XV_Y, x_row - 2,
		XV_HELP_DATA, "cm:ApptTimeUnit",
		0);

	t =  MGET("Stop:") ;
	x_row = xv_get(e->ampm, XV_Y) + xv_get(e->ampm, XV_HEIGHT) + y_gap;
	xv_create(e->panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, t,
		PANEL_LABEL_BOLD, TRUE,
		XV_X, wd - size3.x,
		XV_Y, x_row,
		XV_HELP_DATA, "cm:ApptDuration",
        	0);
	e->durationstack = xv_create(e->panel, PANEL_ABBREV_MENU_BUTTON,
		XV_X, xv_get(e->timestack, XV_X),
		XV_Y, x_row,
		XV_HELP_DATA, "cm:ApptDuration",
		0);

	/*add_duration_strings(p);*/
	xv_set(e->durationstack, PANEL_ITEM_MENU, e_make_duration_menu(e), 0);

	e->duration = xv_create(e->panel, PANEL_TEXT,
		PANEL_VALUE_DISPLAY_LENGTH, 6,
		XV_X, xv_get(e->time, XV_X),
		XV_Y, x_row,
		XV_HELP_DATA, "cm:ApptDuration",
		0);
        e->minhr = xv_create(e->panel, PANEL_CHOICE,
		PANEL_CHOICE_STRINGS,  MGET(" AM ") , MGET(" PM ") , 0,
		XV_X, xv_get(e->ampm, XV_X),
		XV_Y, x_row - 2,
		XV_HELP_DATA, "cm:ApptDurationUnit",
         	0);

	ampm_wid = xv_get(e->ampm, XV_X) + xv_get(e->ampm, XV_WIDTH);
        minhr_wid = xv_get(e->minhr, XV_X) + xv_get(e->minhr, XV_WIDTH);
        if (ampm_wid > minhr_wid)
                tmp = ampm_wid;
        else
                tmp = minhr_wid;

	max_wid = tmp - xv_get(e->timestack, XV_X);

	pvdl = (double)max_wid/pfdef_wid+1;
                
	xv_set(e->datetext, PANEL_VALUE_DISPLAY_LENGTH, pvdl, NULL);

	e_hide_ampm(c);

	t =  MGET("What:");
	y_gap = 4;
	tmp_pi = xv_create(e->panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, t,
		PANEL_LABEL_BOLD, TRUE,
		XV_X, wd - size4.x,
		XV_Y, xv_get(e->minhr, XV_Y) + xv_get(e->minhr, XV_HEIGHT) + y_gap,
		XV_HELP_DATA, "cm:WhatField",
		0);
	e->what[0].item = xv_create(e->panel, PANEL_TEXT,
			XV_WIDTH, max_wid,
			PANEL_VALUE_DISPLAY_LENGTH, pvdl,
			PANEL_VALUE_STORED_LENGTH, WHAT_LEN,
			PANEL_NOTIFY_PROC, what_notify,
			PANEL_NOTIFY_LEVEL, PANEL_ALL,
			XV_X, tmp_x,
			XV_Y, xv_get(tmp_pi, XV_Y),
			XV_HELP_DATA, "cm:WhatField",
			0);
	e->what[1].item = xv_create(e->panel, PANEL_TEXT,
			XV_WIDTH, max_wid,
			PANEL_VALUE_DISPLAY_LENGTH, pvdl,
			PANEL_VALUE_STORED_LENGTH, WHAT_LEN,
			PANEL_NOTIFY_PROC, what_notify,
			PANEL_NOTIFY_LEVEL, PANEL_ALL,
			XV_X, tmp_x,
			XV_Y, xv_get(e->what[0].item, XV_Y) +
				xv_get(e->what[0].item, XV_HEIGHT) + y_gap,
			XV_HELP_DATA, "cm:WhatField",
			0);
	e->what[2].item = xv_create(e->panel, PANEL_TEXT,
			XV_WIDTH, max_wid,
			PANEL_VALUE_DISPLAY_LENGTH, pvdl,
			PANEL_VALUE_STORED_LENGTH, WHAT_LEN,
			PANEL_NOTIFY_PROC, what_notify,
			PANEL_NOTIFY_LEVEL, PANEL_ALL,
			XV_X, tmp_x,
			XV_Y, xv_get(e->what[1].item, XV_Y) + 
				xv_get(e->what[1].item, XV_HEIGHT) + y_gap,
			XV_HELP_DATA, "cm:WhatField",
			0);
	e->what[3].item = xv_create(e->panel, PANEL_TEXT,
			PANEL_VALUE_DISPLAY_LENGTH, pvdl,
			PANEL_VALUE_STORED_LENGTH, WHAT_LEN,
			PANEL_NOTIFY_PROC, what_notify,
			PANEL_NOTIFY_LEVEL, PANEL_ALL,
			XV_X, tmp_x,
			XV_Y, xv_get(e->what[2].item, XV_Y) +
				xv_get(e->what[2].item, XV_HEIGHT) + y_gap,
			XV_HELP_DATA, "cm:WhatField",
			0);
	e->what[4].item = xv_create(e->panel, PANEL_TEXT,
			PANEL_VALUE_DISPLAY_LENGTH, pvdl,
			PANEL_VALUE_STORED_LENGTH, WHAT_LEN,
			PANEL_NOTIFY_PROC, what_notify,
			PANEL_NOTIFY_LEVEL, PANEL_ALL,
			XV_SHOW,    FALSE,
			XV_X, tmp_x,
			XV_Y, xv_get(e->what[3].item, XV_Y) + 
				xv_get(e->what[3].item, XV_HEIGHT) + y_gap,
			XV_HELP_DATA, "cm:WhatField",
			0);

	for (i=0; i<5; i++) 
		e->what[i].itemval[0] = NULL;

	t =  MGET("Alarm:") ;
	x_row = xv_get(e->what[3].item, XV_Y) + 
				xv_get(e->what[3].item, XV_HEIGHT) + 15;
	tmp_pi = xv_create(e->panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, t,
		PANEL_LABEL_BOLD, TRUE,
		XV_X, wd - size5.x,
		XV_Y, x_row,
		XV_HELP_DATA, "cm:ReminderToggle",
		0);

	e->reminder = xv_create(e->panel, PANEL_TOGGLE,
		PANEL_CHOICE_STRINGS,  LGET("Beep") ,  LGET("Flash") ,  
				LGET("PopUp") ,  LGET("Mail") ,  0,
		XV_X, tmp_x,
		XV_Y, xv_get(tmp_pi, XV_Y), 
		PANEL_LAYOUT, PANEL_VERTICAL,
		PANEL_FEEDBACK, PANEL_INVERTED,
		PANEL_NOTIFY_PROC, reminder_notify,
		PANEL_CLIENT_DATA, c,
		XV_HELP_DATA, "cm:ReminderToggle",
		0);

	x_gap = 12;
	y_gap = xv_get(e->reminder, XV_HEIGHT)/4 + 1;
	e->beepadvance = xv_create(e->panel, PANEL_TEXT,
		PANEL_VALUE_DISPLAY_LENGTH, 3,
		XV_X, xv_get(e->reminder, XV_X)  + 
			xv_get(e->reminder, XV_WIDTH) + x_gap, 
		XV_Y, xv_get(e->reminder, XV_Y) + 8,
		PANEL_NOTIFY_PROC, validate_num,
		PANEL_NOTIFY_LEVEL, PANEL_ALL,
		PANEL_CLIENT_DATA, e,
		XV_HELP_DATA, "cm:BeepAdvance",
		0);
	e->beepunitstack = xv_create(e->panel, PANEL_ABBREV_MENU_BUTTON,
		XV_X, xv_get(e->beepadvance, XV_X) + 
			xv_get(e->beepadvance, XV_WIDTH) + x_gap,
		XV_Y, xv_get(e->beepadvance, XV_Y),    
                XV_HELP_DATA, "cm:BeepAdvance",
                0);

	e->beepunit = xv_create(e->panel, PANEL_MESSAGE,
		PANEL_VALUE_DISPLAY_LENGTH, 4,
		XV_X, xv_get(e->beepunitstack, XV_X) + 
			xv_get(e->beepunitstack, XV_WIDTH) + x_gap,
		XV_Y, xv_get(e->beepadvance, XV_Y),     
		XV_HELP_DATA, "cm:BeepAdvance",
		0);
	xv_set(e->beepunitstack, PANEL_ITEM_MENU, 
		e_make_unit_menu(e->beepunit), NULL);

	e->flashadvance = xv_create(e->panel, PANEL_TEXT,
		PANEL_VALUE_DISPLAY_LENGTH, 3,
		XV_X, xv_get(e->beepadvance, XV_X),
		XV_Y, xv_get(e->beepadvance, XV_Y) + y_gap,
		PANEL_NOTIFY_PROC, validate_num,
		PANEL_NOTIFY_LEVEL, PANEL_ALL,
		PANEL_CLIENT_DATA, e,
		XV_HELP_DATA, "cm:FlashAdvance",
		0);
	e->flashunitstack = xv_create(e->panel, PANEL_ABBREV_MENU_BUTTON,
                XV_X, xv_get(e->beepunitstack, XV_X),
		XV_Y, xv_get(e->flashadvance, XV_Y),
                XV_HELP_DATA, "cm:FlashAdvance",
                0);
	e->flashunit = xv_create(e->panel, PANEL_MESSAGE,
		PANEL_VALUE_DISPLAY_LENGTH, 4,
		XV_X, xv_get(e->beepunit, XV_X),
		XV_Y, xv_get(e->flashadvance, XV_Y),
		XV_HELP_DATA, "cm:FlashAdvance",
		0);
	xv_set(e->flashunitstack, PANEL_ITEM_MENU,
			 e_make_unit_menu(e->flashunit), NULL);

	e->openadvance = xv_create(e->panel, PANEL_TEXT,
		PANEL_VALUE_DISPLAY_LENGTH, 3,
		XV_X, xv_get(e->beepadvance, XV_X),
		XV_Y, xv_get(e->flashadvance, XV_Y) + y_gap,
		PANEL_NOTIFY_PROC, validate_num,
		PANEL_NOTIFY_LEVEL, PANEL_ALL,
		PANEL_CLIENT_DATA, e,
		XV_HELP_DATA, "cm:OpenAdvance",
		0);
        e->openunitstack = xv_create(e->panel, PANEL_ABBREV_MENU_BUTTON,
                XV_X, xv_get(e->beepunitstack, XV_X),
		XV_Y, xv_get(e->openadvance, XV_Y),  
                XV_HELP_DATA, "cm:OpenAdvance",
                0);
	e->openunit = xv_create(e->panel, PANEL_MESSAGE,
		PANEL_VALUE_DISPLAY_LENGTH, 4,
		XV_X, xv_get(e->beepunit, XV_X),
		XV_Y, xv_get(e->openadvance, XV_Y),  
		XV_HELP_DATA, "cm:OpenAdvance",
		0);
	xv_set(e->openunitstack, PANEL_ITEM_MENU,
                         e_make_unit_menu(e->openunit), NULL);

	e->mailadvance = xv_create(e->panel, PANEL_TEXT,
		PANEL_VALUE_DISPLAY_LENGTH, 3,
		XV_X, xv_get(e->beepadvance, XV_X),
		XV_Y, xv_get(e->openadvance, XV_Y) + y_gap,
		PANEL_NOTIFY_PROC, validate_num,
		PANEL_NOTIFY_LEVEL, PANEL_ALL,
		PANEL_CLIENT_DATA, e,
		XV_HELP_DATA, "cm:MailAdvance",
		0);
        e->mailunitstack = xv_create(e->panel, PANEL_ABBREV_MENU_BUTTON,
                XV_X, xv_get(e->beepunitstack, XV_X),
		XV_Y, xv_get(e->mailadvance, XV_Y),  
                XV_HELP_DATA, "cm:MailAdvance",
                0);
	e->mailunit = xv_create(e->panel, PANEL_MESSAGE,
		PANEL_VALUE_DISPLAY_LENGTH, 4,
		XV_X, xv_get(e->beepunit, XV_X),
		XV_Y, xv_get(e->mailadvance, XV_Y),  
                XV_HELP_DATA, "cm:MailAdvance",
                0);
	xv_set(e->mailunitstack, PANEL_ITEM_MENU, 
                         e_make_unit_menu(e->mailunit), NULL);

	t =  MGET("Mail To:") ;
	y_gap = 9;
        e->to = xv_create(e->panel, PANEL_TEXT,
                PANEL_LABEL_STRING,  t,
                PANEL_VALUE_DISPLAY_LENGTH, pvdl,
                PANEL_VALUE_STORED_LENGTH, BUFSIZ,
		XV_X, wd - size6.x,
		XV_Y, xv_get(e->reminder, XV_Y) + xv_get(e->reminder, XV_HEIGHT) + y_gap + 5,
                XV_HELP_DATA, "cm:MailAddress",
                0);

	/* find longest string */
	t =  MGET("Mins") ;
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        size1.x = dims.width;
        size1.y = dims.height;
        t =  MGET("Hrs") ;
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        size2.x = dims.width;
        size2.y = dims.height;
        t =  MGET("Days") ;
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        size3.x = dims.width;
        size3.y = dims.height;

	wd_2 = max(max(size1.x, size2.x), size3.x);

	alarm_wid = xv_get(e->beepunit, XV_X) + wd_2;
        if (alarm_wid > tmp)
                tmp = alarm_wid;

        col_setting = tmp + 25;

	x_gap = 6;

	e->appt_type = xv_create(e->panel, PANEL_CHOICE,
                PANEL_CHOICE_STRINGS,  
			LGET("  Appointment  "), LGET("   To Do   "), 0,
                PANEL_VALUE, 0,
		XV_X, col_setting,
		XV_Y, xv_row(e->panel, 0) + 10,
		PANEL_CLIENT_DATA, c,
                XV_HELP_DATA, "cm:BrApptTodoToggle",
                0);

	make_dnd(e);
	dnd_full = true;

	wid = xv_get(e->dnd_target, XV_X) - xv_get(e->appt_type, XV_X);  
	/* Dynamically figure out whether the entire panel title is displayed
	 * after translation.  Make sure the list boxes are big enough for the title.
	 */
	t = MGET("          Appointments");
	(void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
	dims.width += 30;    /* 30 is approximately the width of the scroll bar */
	if ( dims.width > wid ) {
		wid = dims.width;
	}
	/* Borrowing the (char *) msg variable here */ 
	msg = MGET("Calendars      Insert Access");
	(void)xv_get(pf, FONT_STRING_DIMS, msg, &dims);
	dims.width += 30;
	if ( dims.width > wid ) {
		wid = dims.width;
	}

	e->apptbox = (Panel_item) xv_create(e->panel, PANEL_LIST,
		PANEL_LIST_DISPLAY_ROWS,	4, 
		PANEL_LIST_WIDTH,    wid,
		XV_X,	col_setting,	
      		XV_Y, xv_row(e->panel, 1) + y_gap + 7,	
		PANEL_CHOOSE_NONE,	TRUE,
		PANEL_NOTIFY_PROC,	apptbox_notify, 
		PANEL_ITEM_MENU,	(Menu)menu_create(NULL),
		PANEL_CLIENT_DATA, c,
		PANEL_LIST_TITLE,  t,
		XV_HELP_DATA,		"cm:BrAppointmentList",
		0);

	e->calbox = (Panel_item) xv_create(e->panel, PANEL_LIST,
		PANEL_LIST_DISPLAY_ROWS,	3, 
		PANEL_LIST_WIDTH, wid,
		XV_X,	col_setting,
      		XV_Y,	xv_get(e->apptbox, XV_Y) +
			xv_get(e->apptbox, XV_HEIGHT) + y_gap,
		PANEL_CHOOSE_ONE,	FALSE,
		PANEL_CHOOSE_NONE,	TRUE,
		PANEL_NOTIFY_PROC,	calbox_notify, 
		PANEL_ITEM_MENU,	(Menu)menu_create(NULL),
		PANEL_CLIENT_DATA,	 c,
		PANEL_LIST_TITLE, 	 msg,
		XV_HELP_DATA,		"cm:CalList",
		0);

	t =  MGET("Repeat:") ;
	(void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size1.x = dims.width;
	size1.y = dims.height;
	t =  MGET("For:") ;
	(void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size2.x = dims.width;
	size2.y = dims.height;
	t =  MGET("Privacy:") ;
	(void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size3.x = dims.width;
	size3.y = dims.height;

	wd = max(max(size1.x, size2.x), size3.x) + col_setting;

	x_row = xv_get(e->calbox, XV_Y) + 
		xv_get(e->calbox, XV_HEIGHT) + 10,
	t =  MGET("Repeat:") ;
	tmp_pi = xv_create(e->panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, t, 
		PANEL_LABEL_BOLD, TRUE,
		XV_X, wd - size1.x,
      		XV_Y, x_row, 
		XV_HELP_DATA, "cm:RepeatStyle",
         	0);
	stack_x = xv_get(tmp_pi, XV_X) +
                        xv_get(tmp_pi, XV_WIDTH) + x_gap;
	
        e->periodstack = xv_create(e->panel, PANEL_ABBREV_MENU_BUTTON,
		PANEL_ITEM_MENU, mb_make_period_menu(e),
		XV_X, stack_x, 
		XV_Y, x_row,
		PANEL_CLIENT_DATA, c,
		XV_HELP_DATA, "cm:RepeatStyle",
		0);

	tmp_x =  xv_get(e->periodstack, XV_X) + stack_rect->r_width + x_gap;
	e->periodunit = xv_create(e->panel, PANEL_MESSAGE,
                PANEL_VALUE_DISPLAY_LENGTH, 4,
		XV_X, tmp_x,
		XV_Y, x_row,
		PANEL_CLIENT_DATA, c,
                XV_HELP_DATA, "cm:RepeatUnit",
                0);
	t =  MGET("For:") ;
	e->scopestr = xv_create(e->panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, t, 
		PANEL_LABEL_BOLD, TRUE,
		XV_X, wd - size2.x,
		XV_Y, xv_get(tmp_pi, XV_Y) +
			 2*xv_get(tmp_pi, XV_HEIGHT),
		XV_HELP_DATA, "cm:RepeatTimes",
         	0);
	e->scopestack = xv_create(e->panel, PANEL_ABBREV_MENU_BUTTON,
                XV_X, stack_x,
                XV_Y, xv_get(e->scopestr, XV_Y),
                PANEL_ITEM_MENU, mb_make_scope_menu(e),
		XV_HELP_DATA, "cm:RepeatTimes",
                0);
	e->scope = xv_create(e->panel, PANEL_TEXT,
		PANEL_VALUE_DISPLAY_LENGTH, 8,
		XV_X, tmp_x,
		XV_Y, xv_get(e->scopestr, XV_Y),
		XV_HELP_DATA, "cm:RepeatTimes",
		0);

	rect = (Rect *) xv_get(e->scope, PANEL_ITEM_RECT, 0);
	e->scopeunit = xv_create(e->panel, PANEL_MESSAGE,
                PANEL_LABEL_BOLD, TRUE,
                XV_X, xv_get(e->scope, XV_X) + rect->r_width + 2,
		XV_Y, xv_get(e->scopestr, XV_Y),
		XV_HELP_DATA, "cm:RepeatTimes",
                0);
	t =  MGET("Privacy:") ;
	tmp_pi = xv_create(e->panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, t, 
		PANEL_LABEL_BOLD, TRUE,
		XV_X, wd - size3.x,
		XV_Y, xv_get(e->scopestr, XV_Y) +
			 2*xv_get(e->scopestr, XV_HEIGHT),
		XV_HELP_DATA, "cm:PrivacyMenu",
         	0);
        e->privacystack = xv_create(e->panel, PANEL_ABBREV_MENU_BUTTON,
		PANEL_ITEM_MENU, mb_make_priv_menu(e),
		PANEL_VALUE, privacystr[(int)e->privacyval],
		XV_X, stack_x,
                XV_Y, xv_get(tmp_pi, XV_Y),
		PANEL_CLIENT_DATA, c,
		XV_HELP_DATA, "cm:PrivacyMenu",
		0);
	e->privacyunit = xv_create(e->panel, PANEL_MESSAGE,
                PANEL_VALUE_DISPLAY_LENGTH, 4,
		XV_X, tmp_x,
                XV_Y, xv_get(tmp_pi, XV_Y),
		PANEL_CLIENT_DATA, c,
                XV_HELP_DATA, "cm:PrivacyMenu",
                0);

	insert_button = xv_create(e->panel, PANEL_BUTTON,
		PANEL_LABEL_STRING,  LGET("  Insert  ") ,
		PANEL_LABEL_BOLD, TRUE,
		PANEL_NOTIFY_PROC, e_insert_proc,
		PANEL_CLIENT_DATA, c,
		XV_Y, xv_get(e->to, XV_Y) + xv_get(e->to, XV_HEIGHT) + y_gap + 26,
		XV_HELP_DATA, "cm:mbInButton",
		0);

	delete_button = xv_create(e->panel, PANEL_BUTTON,
		PANEL_LABEL_STRING,  LGET(" Delete ") ,
		PANEL_LABEL_BOLD, TRUE,
		PANEL_NOTIFY_PROC, e_delete_proc,
		PANEL_CLIENT_DATA, c,
		XV_Y, xv_get(insert_button, XV_Y),
		XV_HELP_DATA, "cm:mbDelButton",
		0);

	change_button = xv_create(e->panel, PANEL_BUTTON,
		PANEL_LABEL_STRING,  LGET(" Change ") ,
		PANEL_LABEL_BOLD, TRUE,
		PANEL_NOTIFY_PROC, e_change_proc, 
		PANEL_CLIENT_DATA, c,
		XV_Y, xv_get(insert_button, XV_Y),
		XV_HELP_DATA, "cm:mbChButton",
		0);

	reset_button = xv_create(e->panel, PANEL_BUTTON,
		PANEL_LABEL_STRING,  LGET(" Reset ") ,
		PANEL_LABEL_BOLD, TRUE,
		PANEL_NOTIFY_PROC, e_reset_proc, 
		PANEL_CLIENT_DATA, c,
		XV_Y, xv_get(insert_button, XV_Y),
		XV_HELP_DATA, "cm:EdClearButton",
		0);

	(void)xv_set(e->panel, PANEL_DEFAULT_ITEM, insert_button, 0);

	/* find longest string */
	t =  MGET("One Time") ;
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        size1.x = dims.width;
        size1.y = dims.height;
        t =  MGET("Show Time and Text") ;
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        size2.x = dims.width;
        size2.y = dims.height;
        t =  MGET("Show Time Only") ;
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        size3.x = dims.width;
        size3.y = dims.height;
        t =  MGET("Show Nothing") ;
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        size4.x = dims.width;
        size4.y = dims.height;

	wd_2 = max(max(max(size1.x, size2.x), size3.x), size4.x);

	i = xv_get(e->apptbox, XV_X) + xv_get(e->apptbox, XV_WIDTH);
	if (tmp < i) 
		tmp = i;
	i = xv_get(e->dnd_target, XV_X) + xv_get(e->dnd_target, XV_WIDTH);
	if (tmp < i) 
		tmp = i;
	i = xv_get(e->calbox, XV_X) + xv_get(e->calbox, XV_WIDTH);
	if (tmp < i) 
		tmp = i;
	i = xv_get(e->privacystack, XV_X) + xv_get(e->privacystack, XV_WIDTH)
		+ x_gap + wd_2;
	if (tmp < i) 
		tmp = i;

	xv_set(e->panel, XV_WIDTH, tmp + 14, NULL);
	xv_set(e->panel, XV_HEIGHT, xv_get(insert_button, XV_Y) + 
		xv_get(insert_button, XV_HEIGHT) + 3, NULL);

	ds_center_items(e->panel, -1, insert_button, delete_button, 
			change_button, reset_button, NULL);

	window_fit(e->frame);

	set_defaults(e);
	set_default_mail(c);
	ds_position_popup(c->frame, e->frame, DS_POPUP_LOR);
}

extern void
activate_scope(d)
	Editor *d;
{
	switch((int)d->periodval) {
	case single:
		xv_set(d->scope, PANEL_VALUE, "", 0);
		xv_set(d->scope, PANEL_INACTIVE, TRUE, 0);
		xv_set(d->scopestr, PANEL_INACTIVE, TRUE, 0);
		xv_set(d->scopestack, PANEL_INACTIVE, TRUE, 0);
		xv_set(d->scopeunit, PANEL_INACTIVE, TRUE, 0);
		break;
	default:
		xv_set(d->scope, PANEL_INACTIVE, FALSE, 0);
		xv_set(d->scopestr, PANEL_INACTIVE, FALSE, 0);
		xv_set(d->scopestack, PANEL_INACTIVE, FALSE, 0);
		xv_set(d->scopeunit, PANEL_INACTIVE, FALSE, 0);
		break;
	}
}

static void
set_editor_scope(e, p)
Editor *e;
Appt *p;
{
	char buf[20];

	if (p->ntimes == CMFOREVER)
                xv_set(e->scope, PANEL_VALUE, LGET("forever") , 0);
        else {
                switch(p->ntimes) {
                case 0:
                        xv_set(e->scope, PANEL_VALUE, "", 0);
                        break;
                default:
                        sprintf(buf, "%d", p->ntimes);
                        xv_set(e->scope, PANEL_VALUE, buf, 0);
                        break;
                }
        }
}
extern void
set_appt2editor(e, a, showauthor)
        Editor *e;
        Appt *a;
	Boolean showauthor;
{
        Attribute *p=NULL;
        char buf[40];
        int i, unit_type;
        Calendar *c = calendar;
        Props *pr = (Props*)c->properties;
	int reminder_mask=0;
 
        p = (Attribute*)a->attr;
        if (p==NULL) {
                xv_set(e->beepadvance, PANEL_VALUE, "", 0);
                xv_set(e->flashadvance, PANEL_VALUE,"", 0);
                xv_set(e->openadvance, PANEL_VALUE, "", 0);
                xv_set(e->mailadvance, PANEL_VALUE, "", 0);
        }
	else {
        	for (; p != NULL; p = p->next) {
                	if (strcmp(p->attr, "bp")==0) {
                        i = seconds_to_units(pr->beepunit_VAL, atoi(p->value), &unit_type );
                        sprintf(buf, "%d", i);
                        reminder_mask = reminder_mask | bp_mask;
                        xv_set(e->beepadvance, PANEL_VALUE, buf, 0);
			xv_set(e->beepunit, PANEL_LABEL_STRING, unitstr[unit_type], 0);
                	}
                	else if (strcmp(p->attr, "fl")==0) {
                        i = seconds_to_units(pr->flashunit_VAL, atoi(p->value), &unit_type);
                        sprintf(buf, "%d", i);
                        reminder_mask = reminder_mask | fl_mask;
                        xv_set(e->flashadvance, PANEL_VALUE, buf, 0);
			xv_set(e->flashunit, PANEL_LABEL_STRING, unitstr[unit_type], 0);
			}
                	else if (strcmp(p->attr, "op")==0) {
                        i = seconds_to_units(pr->openunit_VAL, atoi(p->value), &unit_type);
                        sprintf(buf, "%d", i);
                        reminder_mask = reminder_mask | op_mask;
                        xv_set(e->openadvance, PANEL_VALUE, buf, 0);
			xv_set(e->openunit, PANEL_LABEL_STRING, unitstr[unit_type], 0);
        	        }
                	else if (strcmp(p->attr, "ml")==0) {
                        i = seconds_to_units(pr->mailunit_VAL, atoi(p->value), &unit_type);
                        sprintf(buf, "%d", i);
                        reminder_mask = reminder_mask | ml_mask;
                        xv_set(e->mailadvance, PANEL_VALUE, buf, 0);
			xv_set(e->mailunit, PANEL_LABEL_STRING, unitstr[unit_type], 0);
			}
		}
        }
        xv_set(e->reminder, PANEL_VALUE, reminder_mask, 0);
	reminder_notify(e->reminder, 0, 0);
	set_editor_scope(e, a);
        if (a->tag->tag == toDo)
                xv_set(e->appt_type, PANEL_VALUE, 1, NULL);
        else
                xv_set(e->appt_type, PANEL_VALUE, 0, NULL);
 
	if (showauthor)
        	sprintf(buf,  MGET("Author: %s") , a->author);
	else
		buf[0] = '\0';
        xv_set(e->frame, FRAME_LEFT_FOOTER, buf, 0);
 
        backup_values(c);
}
static void
add_to_mailto(c, mailname)
        Calendar *c;
        char *mailname;
{
        char *buf, *maillist;
        Editor *e = (Editor*)c->editor;

        maillist = (char*)xv_get(e->to, PANEL_VALUE);
	if (maillist == NULL || *maillist == NULL) 
		xv_set(e->to, PANEL_VALUE, mailname, NULL);
	else {
		buf = (char*)ckalloc(cm_strlen(maillist)+
			cm_strlen(mailname)+2);
		sprintf(buf, "%s %s", maillist, mailname);
		xv_set(e->to, PANEL_VALUE, buf, NULL);
		free(buf);
	}
}

static void
apptbox_action(c, row)

	Calendar *c; 
	int row;
{
	char buf[WHAT_LEN+1];
	Appt *a=NULL;
	Editor *e = (Editor *)c->editor;
	Props *p = (Props *)c->properties;
	int h, i;
	int ampm = 0, num_names=0, mn;
	Lines *lines=NULL, *headlines= NULL;
	struct apptowners *ao=NULL, *aolist=NULL;
	char *tmp=NULL;

	/* get item selected and get names to add to cal box */
	aolist = (struct apptowners*)xv_get(e->apptbox, 
		PANEL_LIST_CLIENT_DATA, row);

	if(aolist->uid == NULL || aolist->uid->appt_id.tick >= EOT) {
		set_defaults(e);
		set_default_calbox(calendar);
		set_default_mail(c);
		return;
	}

	for (ao = aolist; ao != NULL; ao=ao->next) {
		add_to_calbox(c, ao->name);
		num_names++;
	}
	
	if (ca != NULL) {
		destroy_appt(ca);
		ca = NULL;
	}

	table_lookup(aolist->name, aolist->uid, &a); 

	ca = a;
	if (a == NULL) {
		set_defaults(e); 
		set_default_calbox(c);
		set_default_mail(c);
                return;
	}

	/* may be more than one author for multiple appts */
	if (num_names == 1) {  
		sprintf(buf,  MGET("Author: %s") , a->author);
		xv_set(e->frame, FRAME_LEFT_FOOTER, buf, 0);
	}

	i=0;
	whatfilled = false;
	headlines = lines = text_to_lines(a->what, 5);
	for (i=0; i<5; i++) {
		if (lines != NULL) {
			if (lines->s != NULL) {
				sprintf(e->what[i].itemval, "%s", lines->s);
				xv_set(e->what[i].item, PANEL_VALUE, e->what[i].itemval, 0);
				whatfilled = true;
			}
			lines = lines->next;
		}
		else {
			xv_set(e->what[i].item, PANEL_VALUE, "", 0);
		}
	}
	destroy_lines(headlines); headlines=NULL;

	/* set up time values */
	h = hour(aolist->uid->appt_id.tick);
	mn = minute(aolist->uid->appt_id.tick);
	if (p->default_disp_VAL == hour12) {
		if (!adjust_hour(&h)) 
			ampm=1;
		sprintf(buf, "%2d:%02d", h, mn);
		xv_set(e->ampm, PANEL_VALUE, ampm, 0);
	}
	else
		sprintf(buf, "%02d%02d", h, mn);

	if (!a->tag->showtime || magic_time(a->appt_id.tick)) {
		xv_set(e->time, PANEL_VALUE, "", 0);
		xv_set(e->duration, PANEL_VALUE, "", 0);
		startfilled = false;
	}
	else {
		xv_set(e->time, PANEL_VALUE, buf, 0);
		decompose_duration(c, a);
		startfilled = true;
	}
	xv_set(e->periodunit, PANEL_LABEL_STRING,
		period_to_str((int)a->period.period, a->period.nth), NULL);
	xv_set(e->scopeunit, PANEL_LABEL_STRING, repeatstr[(int)a->period.period], 0);
	xv_set(e->privacyunit, PANEL_LABEL_STRING, privacystr[(int)a->privacy], 0);
	e->periodval = a->period.period;
	e->nthval = a->period.nth;
	activate_scope(e); 

	set_appt2editor(e, a, true);

	xv_set(e->to, PANEL_VALUE, "", NULL);
	if (mail_on(e)) {
		for (ao = aolist; ao != NULL; ao=ao->next) {
			if (ao->mailto != NULL && 
				!name_in_list(c, ao->mailto,
				 	xv_get(e->to, PANEL_VALUE), &tmp))
				add_to_mailto(c, ao->mailto);
		}
	}
	else 
		update_mailto(c);

	set_date_on_panel(a->appt_id.tick, e->datetext, 
		p->ordering_VAL, p->separator_VAL);
	datefilled = true;
	set_dnd_target();
}
static void
failed_msg(e, name, op, stat)
	Editor *e;
	char *name;
	Editor_op op;
	Stat stat;
{
	char buf[MAXNAMELEN];

	buf[0] = NULL;

	switch ((Editor_op)op) {
		case DELETION:
			cm_strcpy(buf, EGET("A DELETION Operation Failed."));
			if (stat == status_denied)
				notice_prompt(e->frame, (Event *)NULL,
				NOTICE_MESSAGE_STRINGS,
				MGET("You Do Not Have Delete Access to Calendar:"),
				name,
				0,
				NOTICE_BUTTON_YES,  
				LGET("Continue") ,
				NULL);
			break;
		case INSERTION:
			cm_strcpy(buf, EGET("An INSERTION operation Failed.") );
			if (stat = status_denied)
                		notice_prompt(e->frame, (Event *)NULL,
                    		NOTICE_MESSAGE_STRINGS,
                     		MGET("You Do Not Have Insert Access to Calendar:"),
				name,
                    		0,
                    		NOTICE_BUTTON_YES,  LGET("Continue") ,
                    		NULL);
			break;
		case CHANGE:
			cm_strcpy(buf, EGET("A CHANGE Operation Failed.") );
			if (stat = status_denied)
                		notice_prompt(e->frame, (Event *)NULL,
				NOTICE_MESSAGE_STRINGS,
				MGET("You Do Not Have Access to Calendar:"),
				name,
				0,
				NOTICE_BUTTON_YES,  LGET("Continue") ,
				NULL);
			break;
		case DELETE_FORWARD_NOT_SUPPORTED:
			cm_strcpy(buf, EGET("A DELETION Operation Failed."));
			if (stat == status_notsupported)
				notice_prompt(e->frame, (Event *)NULL,
				NOTICE_MESSAGE_STRINGS,
				MGET("DELETE FORWARD is not available for Calendar:"),
				name,
				MGET("Appointment not Deleted."),
				0,
				NOTICE_BUTTON_YES,  
				LGET("Continue") ,
				NULL);
			break;
		case CHANGE_FORWARD_NOT_SUPPORTED:
			cm_strcpy(buf, EGET("A CHANGE Operation Failed."));
			if (stat == status_notsupported)
				notice_prompt(e->frame, (Event *)NULL,
				NOTICE_MESSAGE_STRINGS,
				MGET("CHANGE FORWARD is not available for Calendar:"),
				name,
				MGET("Appointment not Changed."),
				0,
				NOTICE_BUTTON_YES,  
				LGET("Continue") ,
				NULL);
			break;
		case CHANGE_REPEATING_EVENT_NOT_SUPPORTED:
			cm_strcpy(buf, EGET("A CHANGE Operation Failed."));
			if (stat == status_notsupported)
				notice_prompt(e->frame, (Event *)NULL,
				NOTICE_MESSAGE_STRINGS,
				MGET("This type of repeating event is not"),
				MGET("supported for Calendar:"),
				name,
				MGET("Appointment not Changed."),
				0,
				NOTICE_BUTTON_YES,  
				LGET("Continue") ,
				NULL);
			break;
		case INSERT_REPEATING_EVENT_NOT_SUPPORTED:
			cm_strcpy(buf, EGET("An INSERT Operation Failed."));
			if (stat == status_notsupported)
				notice_prompt(e->frame, (Event *)NULL,
				NOTICE_MESSAGE_STRINGS,
				MGET("This type of repeating event is not"),
				MGET("supported for Calendar:"),
				name,
				MGET("Appointment not Inserted."),
				0,
				NOTICE_BUTTON_YES,  
				LGET("Continue") ,
				NULL);
			break;
		case DELETE_FILEERROR:
			cm_strcpy(buf, EGET("A DELETION Operation Failed."));
			if (stat == status_incomplete)
				notice_prompt(e->frame, (Event *)NULL,
				NOTICE_MESSAGE_STRINGS,
				MGET("You have run out of disk space in /usr/spool/calendar\nor there was a file error accessing your calendar: "),
				name,
				0,
				NOTICE_BUTTON_YES,  
				LGET("Continue") ,
				NULL);
			break;
		case INSERT_FILEERROR:
			cm_strcpy(buf, EGET("An INSERTION Operation Failed."));
			if (stat == status_incomplete)
				notice_prompt(e->frame, (Event *)NULL,
				NOTICE_MESSAGE_STRINGS,
				MGET("You have run out of disk space in /usr/spool/calendar\nor there was a file error accessing your calendar:"),
				name,
				0,
				NOTICE_BUTTON_YES,  
				LGET("Continue") ,
				NULL);
			break;
		case CHANGE_FILEERROR:
			cm_strcpy(buf, EGET("A CHANGE Operation Failed."));
			if (stat == status_incomplete)
				notice_prompt(e->frame, (Event *)NULL,
				NOTICE_MESSAGE_STRINGS,
				MGET("You have run out of disk space in /usr/spool/calendar\nor there was a file error accessing your calendar:"),
				name,
				0,
				NOTICE_BUTTON_YES,  
				LGET("Continue") ,
				NULL);
			break;
	}
	xv_set(e->frame, FRAME_LEFT_FOOTER, buf, NULL);
}
extern void
mb_add_index(start, end)
	int start, end;
{
	struct drawind *ptr, *index_ptr;

	/* indexes out of visible range */
	if (start == -1 || end == -1)
		return;
	/* dont allow duplicates to minimize repainting */
	for(ptr = index_list; ptr != NULL; ptr = ptr->next) {
		if (start == ptr->start && end == ptr->end)
			return;
        }
	index_ptr = (struct drawind*)ckalloc(sizeof(struct drawind));
	index_ptr->start = start;
	index_ptr->end = end;
	index_ptr->next = NULL;

	if (index_list == NULL) {
		index_list = index_ptr;
		return;
	}
	for(ptr = index_list;ptr->next != NULL; ptr = ptr->next);
        ptr->next = index_ptr;
}
extern void
redisplay()
{
        int topoffset, w, h, margin, x, y, hr, yoff, tick;
        Uid *r, *p;
        Calendar *c;
        Props *props;
	Abb_Appt *list=NULL;
	struct Range range;

        c               = calendar;
        w               = c->view->boxw;
        h               = c->view->boxh;
        margin          = c->view->outside_margin;
        topoffset       = margin + c->view->topoffset;
        props           = (Props *) c->properties;


        r = p = interest_list;
	if (r != NULL)
        	calendar_deselect(c);
        while(r != NULL) {
                tick = r->appt_id.tick;
                switch(c->view->glance) {
                case monthGlance:
                        x = w*dow(tick)+margin+3;
                        y = h*(wom(tick)-1) + topoffset+3;
                        yoff = y + xv_get(c->fonts->lucida12b, 
				FONT_DEFAULT_CHAR_HEIGHT)+
				 xv_get(c->fonts->lucida9,
                                FONT_DEFAULT_CHAR_HEIGHT);
			range.key1 = lowerbound(tick);
			range.key2 = next_ndays(tick, 1);
			range.next = NULL;
			table_abbrev_lookup_range(c->view->current_calendar, 
					&range, &list);
                        paint_day_entries(tick, x, yoff-2, list, NULL);
			calendar_select(c, daySelect, (caddr_t)NULL);
                break;
                case dayGlance:
                        hr = hour(tick);
			if (hr < props->begin_slider_VAL ||  
				hr > props->end_slider_VAL) {
                		p = r->next;
                		free(r);
                		r=p;
				continue;
			}
			y = c->view->topoffset + 2 + 
				h * (hr - props->begin_slider_VAL);
			(void)paint_dayview_hour(c, lower_bound(hr, 
				c->view->date), y, NULL); 
                break;
                case weekGlance:
			wk_update_entries(c, r->appt_id.tick);
			calendar_select(c, weekdaySelect, (caddr_t)NULL);
			weekchart_select(c);
                break;
                }
                p = r->next;
                free(r);
                r=p;
        }
        interest_list=NULL;
}

extern void
add_interest(tick)
	int tick;
{
	static Uid *e;
 	Uid	 *p;


	/* dont allow duplicates to minimize repainting */
	for (p = interest_list; p != NULL; p = p->next)
		if (p->appt_id.tick == tick)
			return;
	
	if((e = make_keyentry())==NULL) return;
	e->appt_id.tick = tick;
	p = interest_list;
	if(p==NULL) {
		interest_list=e;
		return;
	}
	while(p->next != NULL) {
		p=p->next;
	}
	p->next = e;
}

extern void
mb_redisplay(b)
	Browser *b;
{
	struct drawind *ind, *save;

	ind = index_list;
	while (ind != NULL) {
		(void)mb_draw_appts(b, ind->start, ind->end);
		save = ind;
		ind = ind->next;
		free(save);
	}
	(void)mb_draw_chartgrid(b);
	index_list=NULL;
}
extern Boolean
showing(tick)
        Tick tick;
{
        int m, mm, y, yy;
        Tick current, day_of_week, montick, suntick;
        struct tm *tm;
	Glance	glance;
	Boolean	is_showing = false;

        current = calendar->view->date;
        tm      = localtime(&tick);
        m       = tm->tm_mon; y = tm->tm_year;
        tm      = localtime(&current);
        mm      = tm->tm_mon; yy = tm->tm_year;
	
	glance = calendar->view->glance;

	switch (glance) {
		case monthGlance:
         		if ((m == mm) && (y == yy))
				is_showing = true;
			break;
		case dayGlance:
			if (m == mm && y == yy && 
				dow(current) == dow(tick))
				is_showing = true;
			break;
		case weekGlance:
			if (y == yy) {
				if ((day_of_week = dow(current)) == 0)
					day_of_week = 7;
				montick = last_ndays(current, day_of_week-1);
				suntick = next_ndays(current, 8-day_of_week); 
				if (tick >= montick && tick <= suntick)
					is_showing = true;
			}
			break;
		case yearGlance:
			is_showing = false;
			break;
		default:
			is_showing = true;
			break;
	}
	return is_showing;
}

static void
update_display(b, p, refresh_cm, addit)
	Browser *b;
	Appt *p;
	Boolean refresh_cm, addit;
{
	int start_ind, end_ind;

	/* update current calendar */
	if (refresh_cm && showing(p->appt_id.tick)) 
		(void)add_interest (p->appt_id.tick);
	/* update browser */
	if (browser_showing(b) && showing_browser(b, p->appt_id.tick)) {
		b->add_to_array = addit;
		(void)mb_update_segs(b, p->appt_id.tick, p->duration, 
				&start_ind, &end_ind);
		(void)mb_add_index(start_ind, end_ind);
	}
}
/* this routine returns the true boundaries of a particular view,i.e,
12midnight - 11:59:59pm.  It does not return the boundaries for doing 
lookups, i.e., 11:59:59pm-12pm (lookup ranges must encompass true range */
extern void
get_range(glance, date, keyrange)
	Glance glance;
	Tick date;
	Keyrange *keyrange;
{
	int day_of_week;
	
	switch(glance) {
		case monthGlance:
			keyrange->tick1 = lowerbound(last_ndays(date, dom(date)-1))+1;
			keyrange->tick2 = next_ndays(keyrange->tick1, monthdays[month(date)-1])-1;
			break;
		case weekGlance:
			day_of_week = (((day_of_week = dow(date)) == 0) 
					? 6 : --day_of_week);
			keyrange->tick1 = lowerbound(date - (day_of_week * daysec))+1;
			keyrange->tick2 = next_ndays(keyrange->tick1, 7)-1;
			break;
		case dayGlance:
			keyrange->tick1 = lowerbound(date)+1;
			keyrange->tick2 = next_nhours(keyrange->tick1, 24)-1;
			break;
		case yearGlance:
			keyrange->tick1 = lowerbound(jan1(date))+1;
			keyrange->tick2 = lowerbound(nextjan1(date));
			break;
		default:
			keyrange->tick1 = lowerbound(jan1(date))+1;
			keyrange->tick2 = lowerbound(nextjan1(date));
			break;
	}	
}
extern void
update_brlist(c, brlist, addit)
	Calendar *c;
	Abb_Appt *brlist;
	Boolean addit;
{
	Abb_Appt *a = NULL;
	Browser *b = (Browser*)c->browser;
	int start_ind, end_ind;

	/* add appts to index list */
	for(a = brlist; a != NULL;  a = a->next)  {
		b->add_to_array = addit;
		mb_update_segs(b, a->appt_id.tick, a->duration, 
				&start_ind, &end_ind); 
		mb_add_index(start_ind, end_ind); 
	}
	destroy_abbrev_appt(brlist);
}
static void
do_lookup(target, keyrange, list)
        char *target;
        Keyrange *keyrange;
        Abb_Appt **list;
{
        Range     *range;
        Abb_Appt *a=NULL, *last_appt=NULL, *tmp_list= NULL;

        /* initialize */
        if (list != NULL) *list = NULL;
        range = (Range*)ckalloc(sizeof(Range));
        range->key1 = keyrange->tick1;
        range->key2 = keyrange->tick2;
        range->next = NULL;
        /* target name already mapped */
        table_abbrev_lookup_range(target, range, &tmp_list);
        for (a = tmp_list; a != NULL; a = a->next) {
                if (a->appt_id.key == keyrange->key) {
                        if (*list == NULL) {
                                *list = copy_single_abbrev_appt(a);
                                last_appt = *list;
                        }
                        else { /* link in new appt */
                                last_appt->next = copy_single_abbrev_appt(a);
                                last_appt = last_appt->next;
                        }
                }
        }
        free(range);
        destroy_abbrev_appt(tmp_list);
}
static void
setup_brlist(name, c, p, brlist)
	char *name;
	Calendar *c;
	Appt *p;
	Abb_Appt **brlist; /* return lists */
{
	Keyrange keyrange;
	Browser *b = (Browser*)c->browser;

	/* initialize */
	if (brlist != NULL) *brlist = NULL;
	/* get browser list */
	keyrange.key = p->appt_id.key;
	keyrange.next = NULL;
	get_range((Glance)(weekGlance), b->date, &keyrange);
	/* range values must encompass lookup range */
	keyrange.tick1--; keyrange.tick2++;
	if (get_data_version(name) >= CMS_VERS_3) 
		table_abbrev_lookup_key_range(name, &keyrange, brlist);
	else 
		do_lookup(name, keyrange, brlist); 
}
static Boolean
do_delete(c, alert_res, name, p)
	Calendar *c;
	int alert_res;
	char *name;
	Appt **p;
{
	Editor *e = (Editor *) c->editor;
	Browser *b = (Browser*)c->browser;
	Abb_Appt *brlist=NULL;
	Boolean delete_ok = false;
	Uidopt uidopt;
	Stat st;

			
	
	if (ca->appt_id.tick < EOT) {
		uidopt.appt_id = ca->appt_id;
		uidopt.option = do_all;
		uidopt.next = NULL;
		/* delete all in repeating series */
		if (alert_res == NOTICE_NO) {
			if (browser_showing(b))
				setup_brlist(name, c, ca, &brlist);
		}
		/* delete one in repeating series */
		else if (alert_res == NOTICE_YES)
			uidopt.option = do_one;
		/* delete forward in repeating series */
		else if (alert_res == NOTICE_FORWARD) {
			if (browser_showing(b))
				setup_brlist(name, c, ca, &brlist);
			uidopt.option = do_forward;
		}
			
		st = table_delete(name, &uidopt, p);
	}
	if (st != status_ok) {
		if (st == status_denied) 
			failed_msg(e, name, DELETION, st);
		else if (st == status_notsupported) 
			failed_msg(e, name, DELETE_FORWARD_NOT_SUPPORTED, 
					st);
		else if (st == status_incomplete)
			failed_msg(e, name, DELETE_FILEERROR, st);
		if (brlist != NULL)
			destroy_abbrev_appt(brlist);
		return delete_ok;
	}
	delete_ok = true;
	if (*p == NULL) 
		*p = copy_appt(ca);
	if (alert_res == NOTICE_NO || alert_res == NOTICE_FORWARD) {
		/* update cm and browser */
		if (browser_showing(b))
			update_brlist(c, brlist, false);
	}
	else
		update_display(b, *p, strcmp(name, c->view->current_calendar) == 0, false);

	return delete_ok;
}
/* store name, uid and mailto field as client data off appts in appt box */
static void
store_names(box, a, row, name, uid)
	Panel_item box;
	Appt *a;
	int row;
	char *name;
	Uid *uid;
{
	struct apptowners *ao, *newa;
	Attribute *p=NULL;
	int i;

	newa = (struct apptowners *) ckalloc(sizeof(struct apptowners));
	newa->name =  name;
	newa->uid = uid;
	p = a->attr;
	if (p != NULL) {
		if (strcmp(p->attr, "ml") == 0 && p->clientdata != NULL)
			 newa->mailto = cm_strdup(p->clientdata);
	}
	ao = (struct apptowners *)xv_get(box, PANEL_LIST_CLIENT_DATA, row);
	newa->next = ao;
	if (ao != NULL)
		ao->prev = newa;
	newa->prev = NULL;
	xv_set(box, PANEL_LIST_CLIENT_DATA, row, newa, NULL);
}

static void
add_to_apptbox(e, a, entry_text, buf)
	Editor *e;
	Appt        *a;
	char *entry_text, *buf;
{
	Uid     *client_data;
	int numitems, i;
	char *name;
	Props *p = (Props*)calendar->properties;

	name = cm_strdup(entry_text);
	format_appt(a, buf, p->default_disp_VAL);
	client_data = (Uid*)ckalloc(sizeof(Uid));
	client_data->appt_id.tick = a->appt_id.tick;
	client_data->appt_id.key = a->appt_id.key;
	client_data->next = NULL;
	numitems = (int)xv_get(e->apptbox, PANEL_LIST_NROWS);

	/* if only one name, show duplicate entries */
	if (xv_get(e->calbox, PANEL_LIST_NROWS) <= 1) {
		list_add_entry(e->apptbox, buf, NULL, 0, numitems, TRUE);
		store_names(e->apptbox, a, numitems, name, client_data);
		free(buf);
		return;
	}
	for (i = xv_get(e->apptbox, PANEL_LIST_NROWS)-1; i >= 0; i--)
		if (strcmp(buf, (char*)xv_get(e->apptbox,
			 PANEL_LIST_STRING, i))==0) {
			store_names(e->apptbox, a, i, name, client_data);
			free(buf);
			return;
		}
	list_add_entry(e->apptbox, buf, NULL, 0, numitems, TRUE);
	store_names(e->apptbox, a, numitems, name, client_data);
	free(buf);
}
/* ARGSUSED */
extern void
mb_add_times(table)
        Panel_item table;
{
	int k, i, j;
	char *buf, *name;
	Appt *a=NULL, *head=NULL;
	Editor *e;
	Props *p;
	Browser *b;
	Calendar *c = calendar;
	Range range;
	Lines *lines=NULL;

	e = (Editor *) c->editor;
	p = (Props *) c->properties;
	b = (Browser *) c->browser;

	if (!editor_exists(e)) return;
	xv_set(table, XV_SHOW, FALSE, NULL);
        range.key1 = b->begin_day_tick;
        range.key2 = b->end_hr_tick;
        range.next = NULL;

	e_list_flush(table);

	for (j = 0, i = xv_get(e->calbox, PANEL_LIST_NROWS); j < i; j++) {
		name = (char*)xv_get(e->calbox, PANEL_LIST_CLIENT_DATA, j); 
		table_lookup_range(name, &range, &a);
		head = a;
		while(a != NULL) { 
			if (a->appt_id.tick+1 <= b->end_hr_tick &&
			(a->appt_id.tick+(a->duration != 0 ? 
					a->duration-1 : a->duration)) 
					>= b->begin_hr_tick) {
				lines = text_to_lines(a->what,1);
				if (lines != NULL && lines->s != NULL) {
					buf = (char *) ckalloc(cm_strlen(lines->s) + 18); 
					destroy_lines(lines); lines=NULL;
				}
				else
					buf = (char *) ckalloc(15); 
				add_to_apptbox(e, a, name, buf);
			}
			a = a->next;
		}
		if (head != NULL) {
			destroy_appt(head); 
			head = NULL;
		}
	}
	xv_set(table, XV_SHOW, TRUE, NULL);
}
static void
rm_name_from_aolist(box, aolist, ao, row)
	Panel_item box;
	struct apptowners **aolist, *ao;
	int row;
{
	if (ao->prev != NULL)
		(ao->prev)->next = ao->next;
	else {
		*aolist = ao->next;
		xv_set(box, PANEL_LIST_CLIENT_DATA, row, *aolist, NULL);
	}
	if (ao->next != NULL)
		(ao->next)->prev = ao->prev;
	free(ao->name);
	free(ao->mailto);
	free(ao->uid);
	free (ao);
	if (*aolist == NULL)
		xv_set(box, PANEL_LIST_CLIENT_DATA, row, 0, NULL);
}
static void
update_ticks(c)
	Calendar *c;
{
	Editor  *e = (Editor*)c->editor;
	Props *p = (Props*)c->properties;
	Browser *b = (Browser*)c->browser;
	char buf[35], *datebuf;

	datebuf = get_date_str(p, e->datetext);
	b->begin_day_tick = cm_getdate(datebuf, NULL)-1;
	sprintf(buf, "%s %s", datebuf, 
		xv_get(e->time, PANEL_VALUE));
	if (p->default_disp_VAL == hour12)
 		(void)cm_strcat(buf, 
			(xv_get(e->ampm, PANEL_VALUE)? "pm":"am"));
	b->begin_hr_tick = cm_getdate(buf, NULL);
	sprintf(buf, "%s %s", datebuf, 
		xv_get(e->duration, PANEL_VALUE));
	if (p->default_disp_VAL == hour12)
 		(void)cm_strcat(buf, 
			(xv_get(e->minhr, PANEL_VALUE)? "pm":"am"));
	b->end_hr_tick = cm_getdate(buf, NULL);
}
static void
refresh_displays(c)
	Calendar *c;
{
	Editor  *e = (Editor*)c->editor;
	Browser *b = (Browser*)c->browser;
	Props *p = (Props*)c->properties;
	int save_begin_day, save_end_hr, save_begin_hr;
	Boolean eshowing = false;

	eshowing = editor_showing(e);
	if (interest_list != NULL) 
		redisplay();
	else
		paint_canvas(c, NULL);
	if (eshowing) {
		xv_set(e->apptbox, XV_SHOW, FALSE, NULL);
		xv_set(e->calbox, XV_SHOW, FALSE, NULL);
		if (browser_showing(b) && 
			c->general->last_canvas_touched == browser) {
				save_begin_day = b->begin_day_tick; 
				save_end_hr = b->end_hr_tick; 
				save_begin_hr = b->begin_hr_tick; 
				update_ticks(c);
				mb_add_times(e->apptbox);
				b->begin_day_tick = save_begin_day; 
				b->end_hr_tick = save_end_hr; 
				b->begin_hr_tick = save_begin_hr; 
		}
		else
                	add_times(e->apptbox);
		xv_set(e->apptbox, XV_SHOW, TRUE, NULL);
		xv_set(e->calbox, XV_SHOW, TRUE, NULL);
	}
	if (browser_showing(b))
		update_browser_display(b, p);
}


/* ARGSUSED */
static Notify_value
e_delete_proc(item, event)
        Panel_item item;
        Event *event;
{
        Calendar *c;
	Editor *e;
	Browser  *b;
        Todo  *t;
        Alist  *a;
        Props  *pr;
        int     row = -1, i = 0, alert_res = -1;
	struct apptowners *ao, *aolist;
	char *name;
	Boolean delete_ok = false;
	Appt *p = NULL;

	c = (Calendar*)xv_get(item, PANEL_CLIENT_DATA);
        b = (Browser *)c->browser;
	e = (Editor*)c->editor;
        t = (Todo*)c->todo;
        a = (Alist*)c->alist;
        pr = (Props*)c->properties;

	if ((row = xv_get(e->apptbox, PANEL_LIST_FIRST_SELECTED)) == -1) {
		notice_prompt(e->frame, (Event *)NULL,
                    NOTICE_MESSAGE_STRINGS,
                     MGET("Select an Appointment and DELETE again.") ,
                    0,
                    NOTICE_BUTTON_YES,  LGET("Continue") ,
                    NULL);
        	return(NOTIFY_DONE);
	} 
	if (xv_get(e->calbox, PANEL_LIST_NROWS) == 0) {
                notice_prompt(e->frame, (Event *)NULL,
                    NOTICE_MESSAGE_STRINGS,
                     MGET("No Calendars Specified for Deletion.\nSelect Reset Button."),
                    0,
                    NOTICE_BUTTON_YES,  LGET("Continue") ,
                    NULL);
                return(NOTIFY_DONE);
	}
	if ((i = xv_get(e->calbox, PANEL_LIST_FIRST_SELECTED)) == -1) {
                notice_prompt(e->frame, (Event *)NULL,
                    NOTICE_MESSAGE_STRINGS,
                     MGET("No Calendars Specified for Deletion.\nSelect Calendar Name in Calendar Box."),
                    0,
                    NOTICE_BUTTON_YES,  LGET("Continue") ,
                    NULL);
                return(NOTIFY_DONE);
	}

	if (( aolist = (struct apptowners*)xv_get(e->apptbox, PANEL_LIST_CLIENT_DATA, row)) 
		== NULL)
		return NOTIFY_DONE;
	
	ao = aolist;
	if (ao->uid != NULL && ao->uid->appt_id.tick < EOT) {
		if (ca != NULL) destroy_appt(ca); ca = NULL;
		table_lookup(ao->name, ao->uid, &ca);
	}
	if (ca != NULL && ca->appt_id.tick < EOT && ca->period.period != single) {
		alert_res = notice_prompt (e->frame, (Event *)NULL,
			NOTICE_MESSAGE_STRINGS, 
			 MGET("This appointment is part of a repeating series.\nDo you want to delete...?") ,
			0,
			NOTICE_BUTTON_YES,  LGET("This One Only"), 
			NOTICE_BUTTON, 	LGET("Forward"), NOTICE_FORWARD, 
			NOTICE_BUTTON_NO,   LGET("All"),
			NOTICE_BUTTON,      LGET("Cancel"), 
			NOTICE_CANCEL,
			0);
	}
	if (alert_res == NOTICE_CANCEL) 
		return(NOTIFY_DONE);
	
	xv_set(e->frame, FRAME_BUSY, TRUE, NULL);
	while (i != -1) {
		name = (char*)xv_get(e->calbox, PANEL_LIST_CLIENT_DATA, i);
                /* get selected name from ao list */
                for (ao = aolist; ao != NULL; ao = ao->next) 
                        if (strcmp(ao->name, name) == 0) 
                                break;
		/* do table lookup to get appt */
                if (ao != NULL && ao->uid != NULL
                        && ao->uid->appt_id.tick < EOT) {
			if (ca != NULL) destroy_appt(ca); ca=NULL;
                        table_lookup(ao->name, ao->uid, &ca);
			if (ca == NULL) {
                        	i = xv_get(e->calbox, 
					PANEL_LIST_NEXT_SELECTED, i);
                        	continue;
			}
		}
                else {
                        i = xv_get(e->calbox, PANEL_LIST_NEXT_SELECTED, i);                    
                        continue;
                }
		if (p != NULL) {
			destroy_appt(p); p = NULL;
		}
		if (do_delete(c, alert_res, ao->name, &p)) {
			delete_ok = true;
			rm_name_from_aolist(e->apptbox, &aolist, ao, row);
		}
		i = xv_get(e->calbox, PANEL_LIST_NEXT_SELECTED, i);
	}


	if (delete_ok) { 
		reset_alarm(c);
		if (browser_showing(b) && 
			c->general->last_canvas_touched == browser)
			browser2calbox(c);
		else 
			set_default_calbox(c);	
		if (p != NULL) {
			refresh_displays(c);
                	if (p->tag->tag == toDo) {
				if (todo_showing(t)) {
					t->changed = true;
                			t_create_todolist(c, t->glance);
				}
			}
			else {
				if (alist_showing(a)) {
					a->changed = true;
                			a_create_alist(c, a->glance);
				}
			}
			destroy_appt(p);
		}
		else {
			add_times(e->apptbox);
			paint_canvas(c, NULL);
			if (browser_showing(b)) {
				update_browser_display2(b, pr);
				list_items_selected(b->box, mb_update_busyicon);
			}
		}
		xv_set(e->frame, FRAME_LEFT_FOOTER, "", 0);
	}
	xv_set(e->frame, FRAME_BUSY, FALSE, NULL);

        return(NOTIFY_DONE);
}

/* ARGSUSED */
extern Notify_value
e_reset_proc(item, event)
	Panel_item item;
	Event *event;
{
	Calendar *c;
	Browser *b;
	Editor *e;
	char *name;
	int i;

	c = calendar;
	b = (Browser*)c->browser;
	e = (Editor*)c->editor;

	if (!editor_exists(e)) return;
	set_defaults(e);
	set_default_calbox(c);
	set_default_mail(c);
	add_times(e->apptbox);

	if (ca != NULL) {
		destroy_appt(ca);
		ca = NULL;
	}

	return NOTIFY_DONE;
}
static void
select_appt_and_reset_calbox(c, a)
	Calendar *c;
	Appt *a;
{
	char *buf=NULL;
	Lines *lines=NULL;
	struct apptowners *ao=NULL, *aolist=NULL;
	Browser *b = (Browser*)c->browser;
	Editor *e = (Editor*)c->editor;
	Props *p = (Props*)c->properties;
	int i;
	Boolean flushed_it = false;

	if (!editor_exists(e)) return;
	/* select recently changed appt */
	lines = text_to_lines(a->what,1);
	if (lines != NULL && lines->s != NULL) {
		buf  = (char*)ckalloc(cm_strlen(lines->s) + 18);
		destroy_lines(lines);
	}
	else
		buf  = (char*)ckalloc(15);
	format_appt(a, buf, p->default_disp_VAL);
	if (ca != NULL) destroy_appt(ca);
	ca = a;

	for (i = xv_get(e->apptbox, PANEL_LIST_NROWS) - 1; i >= 0; i--)
		if (strcmp(buf, (char*)xv_get(e->apptbox, 
			PANEL_LIST_STRING, i)) == 0) {
			xv_set(e->apptbox, PANEL_LIST_SELECT, i, TRUE, NULL);
			/* get names to add to cal box */
			aolist = (struct apptowners*)xv_get( e->apptbox,
					 PANEL_LIST_CLIENT_DATA, i);
			for (ao = aolist; ao != NULL; ao=ao->next) {
				if (!flushed_it) {
					list_cd_flush(e->calbox);
					flushed_it = true;
				}
				add_to_calbox(c, ao->name);
			}
			if (list_num_selected(e->calbox) == 1) {  
				free(buf);
				buf =  (char*)ckalloc(cm_strlen(a->author)+15);
				sprintf(buf,  MGET("Author: %s") , a->author);
				xv_set(e->frame, FRAME_LEFT_FOOTER, buf, 0);
			}
			xv_set(e->periodunit, PANEL_LABEL_STRING, 
				period_to_str((int)a->period.period,
				a->period.nth), 0);
        		xv_set(e->scopeunit, PANEL_LABEL_STRING, 
				repeatstr[(int)a->period.period], 0);
        		e->periodval = a->period.period;
        		e->nthval = a->period.nth;
        		activate_scope(e);
			break;
		}
	free(buf);
}
static Boolean
do_insert(c, name, appt, p)
	Calendar *c;
	char *name;
	Appt *appt;
	Appt **p;
{
	Abb_Appt *brlist = NULL;
	Browser *b = (Browser*)c->browser;
	Editor *e = (Editor*)c->editor;
	Boolean insert_ok = false, no_todo=false;
	Stat st;
	Attribute *attr;
	int dv;

	*p = NULL;
	dv = get_data_version(name);
	if (dv <= CMS_VERS_2)  
		if (appt->tag->tag == toDo) {
			appt->tag->tag = appointment;
			no_todo = true;
		}

	if (mail_on(e)) {
		attr = mailto2appt(c, name);
		if (attr != NULL) {
			attr->next = appt->attr;
			appt->attr = attr;
		}
	}
	if ((st = table_insert(name, appt, p)) != status_ok) {
		if (st == status_denied) {
			failed_msg(e, name, INSERTION, st);
			return insert_ok;
		}
		if (st == status_notsupported) {
			failed_msg(e, name, INSERT_REPEATING_EVENT_NOT_SUPPORTED, st);
			return insert_ok;
		}
		if (st == status_incomplete) {
			failed_msg(e, name, INSERT_FILEERROR, st);
			return insert_ok;
		}
	}
	insert_ok = true;
	if (no_todo)
		notice_prompt(e->frame, (Event *)NULL,
			NOTICE_MESSAGE_STRINGS,
			MGET("The Calendar:"), name,
			MGET("Does Not Support Todo Items."),
			MGET("Entered as an Appointment."),
			0,
			NOTICE_BUTTON_YES,  LGET("Continue") ,
			NULL);
	if (dv <= CMS_VERS_3 && appt->privacy == semiprivate) {
		notice_prompt(e->frame, (Event *)NULL,
			NOTICE_MESSAGE_STRINGS,
			MGET("The Calendar:"), name,
			MGET("Does Not Support Time-Only Appointments."),
			MGET("Entered with Time-and-Text."),
			0,
			NOTICE_BUTTON_YES,  LGET("Continue") ,
			NULL);
	}
	if (*p == NULL)
		return insert_ok;
			
	/* successful insert; add to mbrowse */
	if (appt->period.period == single) 
		update_display(b, *p,
		 strcmp(name, c->view->current_calendar) == 0, true);
	else if (browser_showing(b)) {
		setup_brlist(name, c, *p, &brlist);
		update_brlist(c, brlist, true);
	}

	return insert_ok;
}

/* ARGSUSED */
static Notify_value
e_insert_proc(item, event)
        Panel_item item;
        Event *event;
{
        Appt   *p=NULL,  *a=NULL;
        Calendar*c;
	Editor	*e;
	Browser	*b;
	Alist *al;
	Todo *t;
	Props *pr;
	int alert_res, wk, i = -1;
	char *access, *newname, *name;
	Boolean insert_ok = false;

	c = (Calendar *)xv_get(item, PANEL_CLIENT_DATA);
	b = (Browser*)c->browser;
	e = (Editor*)c->editor;
	al = (Alist*)c->alist;
	t = (Todo*)c->todo;
	pr = (Props*)c->properties;
        
	if ((a = panel_to_appt(c)) == NULL) 
		return false;
	if (xv_get(e->calbox, PANEL_LIST_NROWS) == 0) {
                notice_prompt(e->frame, (Event *)NULL,
                    NOTICE_MESSAGE_STRINGS,
                     MGET("No Calendars Specified for Insertion.\nSelect Reset Button."),
                    0,
                    NOTICE_BUTTON_YES,  LGET("Continue") ,
                    NULL);
                return(NOTIFY_DONE);
	}
	if ((i = xv_get(e->calbox, PANEL_LIST_FIRST_SELECTED)) == -1) {
                notice_prompt(e->frame, (Event *)NULL,
                    NOTICE_MESSAGE_STRINGS,
                     MGET("No Calendar Specified for Insertion.\nSelect Calendar Name in Calendar Box.") ,
                    0,
                    NOTICE_BUTTON_YES,  LGET("Continue") ,
                    NULL);
                return(NOTIFY_DONE);
	}
	/* MonthByWeekdayif and 4th week of month and last week of month */  
	if (a->period.period == nthWeekday) {
		if (weekofmonth(a->appt_id.tick, &wk) && wk == 4) {
			alert_res = notice_prompt(e->frame, (Event *)NULL,
				NOTICE_MESSAGE_STRINGS,
                                MGET("Would you like to schedule this appointment\nas the Last Week of the Month or the\n4th Week of the Month?"),
				0,
				NOTICE_BUTTON_YES,  LGET("Last Week") ,
				NOTICE_BUTTON_NO,   LGET("4th Week") ,
				NOTICE_BUTTON,	    LGET("Cancel"), NOTICE_CANCEL,
				0);
			if (alert_res == NOTICE_CANCEL)
                		return(NOTIFY_DONE);
			if (alert_res == NOTICE_YES)
				a->period.nth = -1;
			else 
				a->period.nth = wk;
		}
		else if (wk == 5)
			a->period.nth = -1;
		else
			a->period.nth = wk;
	}

	xv_set(e->frame, FRAME_BUSY, TRUE, NULL);
	while (i != -1) {
		name = (char*)xv_get(e->calbox, PANEL_LIST_STRING, i);
		str_to_entry(name, &newname, &access);
		if (*access == 'N') {
			failed_msg(e, newname, INSERTION, status_denied);
			i = xv_get(e->calbox, PANEL_LIST_NEXT_SELECTED,i);
			free(newname); free(access);
			continue;
		}
		free(newname); free(access);
		newname = (char*)xv_get(e->calbox, PANEL_LIST_CLIENT_DATA, i);
		if (p != NULL) {
			destroy_appt(p); p = NULL;
		}
		if (do_insert(c, newname, a, &p))
			insert_ok = true;
		i = xv_get(e->calbox, PANEL_LIST_NEXT_SELECTED, i);
	}

	if (insert_ok) {
		reset_alarm(c);
		if (p != NULL) {
			refresh_displays(c);
			/* older version returns old what field */
			if (get_data_version(newname) <= CMS_VERS_3) {
				free(p->what);
				free(p->author);
				p->what = cm_strdup(a->what);
				p->author = cm_strdup((char *)c->user);
			}
			select_appt_and_reset_calbox(c, p);
                	if (p->tag->tag == toDo) {
				if (todo_showing(t)) {
					t->changed = true;
                			t_create_todolist(c, t->glance);
				}
			}
			else {
				if (alist_showing(al)) {
					al->changed = true;
                			a_create_alist(c, al->glance);
				}
			}
		}
		else {
			add_times(e->apptbox);
			paint_canvas(c, NULL);
			if (browser_showing(b)) {
				update_browser_display2(b, pr);
				list_items_selected(b->box, mb_update_busyicon);
			}
		}
	}
	/* dont free p cuz it is now the global "ca" */
	destroy_appt(a); 
	xv_set(e->frame, FRAME_BUSY, FALSE, NULL);

        return(NOTIFY_DONE);
}
static void
update_scope(e)
	Editor *e;
{
	xv_set(e->periodunit, PANEL_LABEL_STRING, periodstr[single], NULL);
	xv_set(e->scope, PANEL_VALUE, "", NULL);
	xv_set(e->scopeunit, PANEL_LABEL_STRING, "", NULL);
        e->periodval = single; 
        e->nthval = 0; 
	activate_scope(e);
	xv_set(e->scope, PANEL_VALUE, repeatval[(int)e->periodval], 0);
	xv_set(e->scopeunit, PANEL_LABEL_STRING, repeatstr[(int)e->periodval], 0);
}
static Boolean
do_change(c, appt, name, alert_res, p)
	Calendar *c;
        Appt *appt;
	char *name;
        int alert_res;
	Appt    **p;
{
	Boolean change_ok = false, no_todo = false;
	Stat st;
	Attribute *attr;
	Editor *e = (Editor*)c->editor;
	Options opt = do_all;

	*p = NULL;
	if (get_data_version(name) <=  CMS_VERS_2)
                if (appt->tag->tag == toDo) {
                        appt->tag->tag = appointment;
                        no_todo = true;
        }
	if (mail_on(e)) {
                attr = mailto2appt(c, name);
		if (attr != NULL) {
                	attr->next = appt->attr;
                	appt->attr = attr;
		}
        }
 
	/* change one instance of repeating series */
	if (alert_res == NOTICE_YES) {
		opt = do_one;

		/* this check should be done by the frontend */
		if (ca->period.period == appt->period.period &&
		    ca->ntimes == appt->ntimes) {
			appt->period.period = single;
			appt->ntimes = 0;
		}

	} else if (alert_res == NOTICE_FORWARD)
		/* change forward of repeating series */
		opt = do_forward;
		
	/* retain status info */
	appt->appt_status = ca->appt_status;

	if ((st = table_change(name, &ca->appt_id, appt, opt, p)) 
				!= status_ok) {
		if (st == status_denied) {
			failed_msg(e, name, CHANGE, st);
			return change_ok;
		}
		if (st == status_notsupported) {
			if (opt == do_forward)
				failed_msg(e, name, 
					CHANGE_FORWARD_NOT_SUPPORTED, st);
			else
				failed_msg(e, name, 
					CHANGE_REPEATING_EVENT_NOT_SUPPORTED, st);
			return change_ok;
		}
		if (st == status_incomplete) {
			failed_msg(e, name, CHANGE_FILEERROR, st);
			return change_ok;
		}
	}
	change_ok = true;

	if (no_todo)
		notice_prompt(e->frame, (Event *)NULL,
                	NOTICE_MESSAGE_STRINGS,
                	MGET("The Calendar:"), name,
                	MGET("Does Not Support Todo Items."),
                	MGET("Entered as an Appointment."),
                	0,
                	NOTICE_BUTTON_YES,  LGET("Continue") ,
                	NULL);

	return change_ok;
}

/* ARGSUSED */
static Notify_value
e_change_proc(item, event)
        Panel_item item;
        Event *event;
{
	Calendar *c = (Calendar*)xv_get(item, PANEL_CLIENT_DATA);
        Editor *e = (Editor*)c->editor;
        Browser  *b = (Browser *)c->browser;
        Todo  *t = (Todo *)c->todo;
        Alist  *al = (Alist *)c->alist;
        Props  *pr = (Props*)c->properties;
	int wk, alert_res = -1, alert_res2 = -1, i = -1, row = -1;
	struct apptowners *ao, *aolist;
	char *bname, *save_mailto;
	Appt *a=NULL, *p=NULL;
	Boolean change_ok = false;
        
	if ((i = xv_get(e->apptbox, PANEL_LIST_FIRST_SELECTED)) == -1) {
                notice_prompt(e->frame, (Event *)NULL,
                    NOTICE_MESSAGE_STRINGS,
                     MGET("Select an Appointment and CHANGE again.") ,
                    0,
                    NOTICE_BUTTON_YES,  LGET("Continue") ,
                    NULL);
                return(NOTIFY_DONE);
        }
	if (xv_get(e->calbox, PANEL_LIST_NROWS) == 0) {
                notice_prompt(e->frame, (Event *)NULL,
                    NOTICE_MESSAGE_STRINGS,
                     MGET("No Calendars Specified for Change.\nSelect Reset Button."),
                    0,
                    NOTICE_BUTTON_YES,  LGET("Continue") ,
                    NULL);
                return(NOTIFY_DONE);
	}
	if (xv_get(e->calbox, PANEL_LIST_FIRST_SELECTED) == -1) {
                notice_prompt(e->frame, (Event *)NULL,
                    NOTICE_MESSAGE_STRINGS,
                     MGET("No Calendars Specified for Changing.\nSelect Calendar Name in Calendar box."),
                    0,
                    NOTICE_BUTTON_YES,  LGET("Continue") ,
                    NULL);
                return(NOTIFY_DONE);
	}
	if ((a = panel_to_appt(c)) == NULL)
		return(NOTIFY_DONE);
	save_mailto = xv_get(e->to, PANEL_VALUE);

	if ((aolist = (struct apptowners*)xv_get(e->apptbox,
			 PANEL_LIST_CLIENT_DATA, i)) == NULL) {
		destroy_appt(a);
		return(NOTIFY_DONE);
	}
	ao = aolist; 
	if (ao->uid == NULL || ao->uid->appt_id.tick >= EOT) return;
	if (ca != NULL) destroy_appt(ca); ca = NULL;
	table_lookup(ao->name, ao->uid, &ca);
	if (ca == NULL) return;

        if (ca->appt_id.tick < EOT && ca->period.period != single) 
		alert_res = notice_prompt(e->frame, (Event *)NULL,
			NOTICE_MESSAGE_STRINGS,
			 MGET("This appointment is part of a repeating series.\nDo you want to change...?") ,
			0,
			NOTICE_BUTTON_YES,  LGET("This One Only") ,
			NOTICE_BUTTON,  LGET("Forward"), NOTICE_FORWARD,
			NOTICE_BUTTON_NO,   LGET("All") ,
			NOTICE_BUTTON,	    LGET("Cancel") , NOTICE_CANCEL,
			0);
	if (alert_res == NOTICE_CANCEL) {
		destroy_appt(a);
		return(NOTIFY_DONE);
	}
	if (a->period.period == nthWeekday) {
                if (weekofmonth(a->appt_id.tick, &wk) && wk == 4) {
                        alert_res2 = notice_prompt(e->frame, (Event *)NULL,
                                NOTICE_MESSAGE_STRINGS,
                                MGET("Would you like to schedule this appointment\nas the Last Week of the Month or the\n4th Week of the Month?"),
                                0,
                                NOTICE_BUTTON_YES,  LGET("Last Week"),
                                NOTICE_BUTTON_NO,   LGET("4th Week"),
				NOTICE_BUTTON,      LGET("Cancel"), NOTICE_CANCEL,
                                0);
                        if (alert_res2 == NOTICE_CANCEL)
                                return(NOTIFY_DONE);
                        if (alert_res2 == NOTICE_YES)
                                a->period.nth = -1;
                        else
                                a->period.nth = wk;
                }
                else if (wk == 5)
                        a->period.nth = -1;
                else
                        a->period.nth = wk;
        }

	xv_set(e->frame, FRAME_BUSY, TRUE, NULL);
	i = xv_get(e->calbox, PANEL_LIST_FIRST_SELECTED);
	while (i != -1) {
		bname = (char*)xv_get(e->calbox, PANEL_LIST_CLIENT_DATA, i);
		/* get selected name from ao list */
		for (ao = aolist; ao != NULL; ao = ao->next)
			if (strcmp(bname, ao->name) == 0) break;

		/* do table lookup to get appt */
		if (ao != NULL && ao->uid != NULL && 
			ao->uid->appt_id.tick < EOT) {
			if (ca != NULL) destroy_appt(ca); ca = NULL;
			table_lookup(ao->name, ao->uid, &ca);
			if (ca == NULL) {
				i = xv_get(e->calbox, PANEL_LIST_NEXT_SELECTED, i);
				continue;
			}
		}
		else {
			i = xv_get(e->calbox, PANEL_LIST_NEXT_SELECTED, i);
			continue;
		}
		if (p != NULL) {
			destroy_appt(p); p = NULL;
		}
		if (do_change(c, a, ao->name, alert_res, &p)) {
			change_ok = true;
		}
		i = xv_get(e->calbox, PANEL_LIST_NEXT_SELECTED, i);
	}

	if (change_ok) {
		reset_alarm(c);
		if (p != NULL) {
			if (get_data_version(ao->name) <= CMS_VERS_3) {
				free(p->what);
				free(p->author);
				p->what = cm_strdup(a->what);
				p->author = cm_strdup((char *)c->user);
			}
			refresh_displays(c);
			if (alert_res == NOTICE_FORWARD)
				set_editor_scope(e, p);
			select_appt_and_reset_calbox(c, p);
                	if (p->tag->tag == toDo) {
				if (todo_showing(t)) {
					t->changed = true;
                			t_create_todolist(c, t->glance);
				}
			}
			else {
				if (alist_showing(al)) {
					al->changed = true;
                			a_create_alist(c, al->glance);
				}
			}
		}
		else {
			add_times(e->apptbox);
			paint_canvas(c, NULL);
			if (browser_showing(b)) {
				update_browser_display2(b, pr);
				list_items_selected(b->box, mb_update_busyicon);
			}
		}
	}
	/* dont destroy p cuz it is now "ca" */
	destroy_appt(a); 
	/* bug 1177936; do reset mailto field to default */
	xv_set(e->to, PANEL_VALUE, save_mailto, NULL);
	xv_set(e->frame, FRAME_BUSY, FALSE, NULL);
	
        return(NOTIFY_DONE);
}
/* used in day view selection and weekglance chart selection;
sets  the hour on the editor */
extern void
set_editor_time(p, hr, e)
Props  *p;
int hr;
Editor *e;
{
        int save_hr;
 
	if (!editor_exists(e) || hr == -1) return;
	if (p->default_disp_VAL == hour12) {
		save_hr = hr;
		if (!adjust_hour(&hr))
			e->ampmval = 1;
		else 
			e->ampmval = 0;
		(void)sprintf(e->timeval, "%d:00", hr);
		(void)xv_set(e->ampm, PANEL_VALUE, e->ampmval, 0);                    
		save_hr++;
		if (!adjust_hour(&save_hr))
			e->minhrval = 1;
		else
			e->minhrval = 0;
		if ( (save_hr == 12) && (e->ampmval == 1) ) {
		/* If start time is 11pm, end time should be 12AM */                           
			e->minhrval = 0;
		}
		(void)xv_set(e->minhr, PANEL_VALUE, e->minhrval, 0);                  
		(void)sprintf(e->durval, "%d:00", save_hr);
	}
	else {   
		(void)sprintf(e->timeval, "%02d00", hr); 
		hr++;
		if (hr >= 24) hr -= 24;
		(void)sprintf(e->durval, "%02d00", hr);
	}
	(void)xv_set(e->time, PANEL_VALUE, e->timeval, 0);
	(void)xv_set(e->duration, PANEL_VALUE, e->durval, 0);
}
extern void
show_editor(c)
        Calendar *c;
{
	int i;
        Editor *e = (Editor *) c->editor;
 
        if (!editor_exists(e))
		make_editor(c);

	(void)xv_set(e->frame, XV_SHOW, TRUE, 0);
        (void) xv_set(e->frame, FRAME_CMD_PUSHPIN_IN, TRUE, 0);
        xv_set(c->frame, FRAME_LEFT_FOOTER, "", 0);
}
	
extern void
reset_time_date_appts(c, hr)
	Calendar *c;
	int hr;
{
	Editor *e = (Editor*)c->editor;
        Props *p = (Props*)c->properties;

	if (!editor_exists(e)) return;
	set_date_on_panel(c->view->date, e->datetext, p->ordering_VAL,
                        p->separator_VAL);
	set_editor_time(p, hr, e);
        add_times(e->apptbox);
        show_editor(c);
}
extern void
reset_date_appts(c)
        Calendar *c;
{
        Editor *e = (Editor*)c->editor;
        Props *p = (Props*)c->properties;

        if (!editor_exists(e)) return;
        set_date_on_panel(c->view->date, e->datetext, p->ordering_VAL,
                        p->separator_VAL);      
        add_times(e->apptbox);
        show_editor(c);
}
extern void
new_editor(m, mi)
        Menu m;
        Menu_item mi;
{
	int i;
        Editor *e = (Editor*)calendar->editor;
	
        if (!editor_exists(e))
                make_editor(calendar);

        if (cal_update_props()) {
                for (i=0; i < NO_OF_PANES; i++) {
                        set_rc_vals((Props*)calendar->properties, i);
                }
                set_defaults(e);
        }

/*	Fix for bug 1181680.	DPT 19/6/96
**	Reversed the order of these two functions.
**	reset_date_appts uses a value set within set_default_calbox
*/
		
        set_default_calbox(calendar);
        reset_date_appts(calendar);
        set_default_mail(calendar);
 
}
extern void
show_appt(a, showauthor)
	Appt *a;
	Boolean showauthor;
{
	char buf[WHAT_LEN+1];
	Editor *e = (Editor *)calendar->editor;
	Props *p = (Props *)calendar->properties;
	int h, ampm = 0, mn, i;
	Lines *lines = NULL, *headlines = NULL;

	if (!editor_exists(e)) return;
	i=0;
	whatfilled = false;
	headlines = lines = text_to_lines(a->what, 5);
	for (i=0; i<5; i++) {
		if (lines != NULL) {
			if (lines->s != NULL) {
				sprintf(e->what[i].itemval, "%s", lines->s);
				xv_set(e->what[i].item, PANEL_VALUE, e->what[i].itemval, 0);
				whatfilled = true;
			}
			lines = lines->next;
		}
		else {
			xv_set(e->what[i].item, PANEL_VALUE, "", 0);
		}
	}
	destroy_lines(headlines); headlines=NULL;

	/* set up time values */
	h = hour(a->appt_id.tick);
	mn = minute(a->appt_id.tick);
	if (p->default_disp_VAL == hour12) {
		if (!adjust_hour(&h)) 
			ampm=1;
		sprintf(buf, "%2d:%02d", h, mn);
		xv_set(e->ampm, PANEL_VALUE, ampm, 0);
	} else
		sprintf(buf, "%02d%02d", h, mn);

	if (!a->tag->showtime || magic_time(a->appt_id.tick)) {
		xv_set(e->time, PANEL_VALUE, "", 0);
		xv_set(e->duration, PANEL_VALUE, "", 0);
		startfilled = false;
	} else {
		xv_set(e->time, PANEL_VALUE, buf, 0);
		decompose_duration(calendar, a);
		startfilled = true;
	}
	xv_set(e->periodunit, PANEL_LABEL_STRING,
		period_to_str((int)a->period.period, a->period.nth), 0);
	xv_set(e->scopeunit, PANEL_LABEL_STRING, repeatstr[(int)a->period.period], 0);
	xv_set(e->privacyunit, PANEL_LABEL_STRING, privacystr[(int)a->privacy], 0);
	e->periodval = a->period.period;
	e->nthval = a->period.nth;
	activate_scope(e); 

	set_appt2editor(e, a, showauthor);
	xv_set(e->to, PANEL_VALUE, "", NULL);

	set_date_on_panel(a->appt_id.tick, e->datetext, 
		p->ordering_VAL, p->separator_VAL);
	datefilled = true;
	set_dnd_target();
}

