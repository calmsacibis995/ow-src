#ifdef lint
static  char sccsid[] = "@(#)todo.c 3.16 94/09/01 Copyr 1991 Sun Microsystems, Inc.";
#endif
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

#include <stdio.h>
#include <xview/xview.h>
#include <xview/frame.h>
#include <xview/font.h>
#include <xview/cms.h>
#include <xview/canvas.h>
#include <xview/panel.h>
#include <xview/notice.h>
#include <xview/xv_xrect.h>
#include <xview/scrollbar.h>
#include "appt.h"
#include "util.h"
#include "timeops.h"
#include "datefield.h"
#include "props.h"
#include "graphics.h"
#include "ps_graphics.h"
#include "calendar.h"
#include "todo.h"
#include "ds_popup.h"
#include "table.h"
#include "gettext.h"

#define TODO_GAP 25
#define MIN_HEIGHT 330

t_destroy_all(t)
        struct Todo_item *t;
{
	if (t == NULL) return;
	t_destroy_all(t->next);
	if (t != NULL) {
		/* set pi's to show false, cux toolkit doesnt and
			display gets messed up */
		xv_set(t->pi, XV_SHOW, FALSE, NULL);
		xv_destroy_safe(t->pi);
		xv_set(t->box_pi, XV_SHOW, FALSE, NULL);
		xv_destroy_safe(t->box_pi);
		xv_set(t->numpi, XV_SHOW, FALSE, NULL);
		xv_destroy_safe(t->numpi);
	}
	free(t);
	t = NULL;
}
extern Boolean
todo_showing(t)
	Todo *t;
{
	if (t != NULL && t->frame != NULL && xv_get(t->frame, XV_SHOW))
		return true;
	return false;
}

static Notify_value
todo_done_proc(frame)
	Frame frame;
{
        Calendar *c;
	Todo *t;
	struct Todo_item *tdi = NULL, *prev_tdi = NULL;
 
	if (frame == NULL) return NOTIFY_DONE;
	c = (Calendar*)xv_get(frame, WIN_CLIENT_DATA);
	t = (Todo*)c->todo;

	for(prev_tdi = tdi = t->head_todo; tdi != NULL; prev_tdi = tdi) {
		tdi = tdi->next;
		free(prev_tdi);
	}

	xv_destroy_safe(t->frame);
	t->frame = NULL;
	t->head_todo = NULL;
	t->last_todo = NULL;
	t->none_pi = NULL;
        return NOTIFY_DONE;
}
static Notify_value
t_event(client, event, arg, when)
        Notify_client     client;
        Event             *event;
        Notify_arg        arg;
        Notify_event_type when;
{
	Calendar *c;
	Todo *t;

	if (event_action(event) == WIN_RESIZE)
        {
		c = (Calendar*)xv_get(client, WIN_CLIENT_DATA);
		t = (Todo*)c->todo;
		xv_set(t->sb, SCROLLBAR_OBJECT_LENGTH, 
				t->num_items,
				SCROLLBAR_VIEW_LENGTH,
			 xv_get(t->frame, XV_HEIGHT) / TODO_GAP,
		NULL);
		
	}
	return (notify_next_event_func(client, (Notify_event)event, arg, when));
}
/* ARGSUSED */
static Notify_value
t_check_box(item, event)
        Panel_item item; 
	Event *event;
{
	int box_pval;
	struct Todo_item  *tdi;
	Appt *a = NULL, *p = NULL;
	Calendar *c = calendar;

	box_pval = (int)xv_get(item, PANEL_VALUE);
	tdi = (struct Todo_item*)xv_get(item, PANEL_CLIENT_DATA);
	(void)table_lookup(c->view->current_calendar, &tdi->uid, &a);
	if (a != NULL) {
		if (box_pval) 
			a->appt_status = completed;
		else
			a->appt_status = active;
		if (a->period.period != single) {
			a->period.period = single;
			a->ntimes = 0;
			table_change(c->view->current_calendar, &tdi->uid, a, do_one, &p);
		}
		else
			table_change(c->view->current_calendar, &tdi->uid, a, do_all, &p);
		destroy_appt(a); destroy_appt(p);
	}
	return NOTIFY_DONE;
}

struct Todo_item *
t_create_todo_item(ptr, t, x, y, a, item_num)
	struct Todo_item *ptr;
        Todo *t;
	int x, y;
	Abb_Appt *a;
	int item_num;
{
	struct Todo_item  *tdi;
	char	buf[101], buf2[10], buf3[115], num_items[20];
	Calendar *c = calendar;
	Props *p = (Props*)c->properties;

	buf[0] = buf2[0] = buf3[0] = '\0';
	(void)format_maxchars(a, buf, 100, p->default_disp_VAL);
	if (t->glance != dayGlance) {
		(void)format_date3(a->appt_id.tick, p->ordering_VAL,
				p->separator_VAL, buf2);
		sprintf(buf3, "%s  %s", buf2, buf);
	}
	else
		cm_strcpy(buf3, buf);
		
	if (ptr == NULL) {
		tdi = (struct Todo_item*)ckalloc(sizeof(struct Todo_item));
		if (item_num > 9)	
			sprintf(num_items, "%d.", item_num);
		else
			sprintf(num_items, " %d.", item_num);
		tdi->uid.appt_id.tick = a->appt_id.tick;
		tdi->uid.appt_id.key = a->appt_id.key;
		tdi->uid.next = NULL; 
		tdi->numpi = xv_create(t->panel, PANEL_MESSAGE,
			PANEL_LABEL_STRING, num_items,
			PANEL_LABEL_BOLD, TRUE,
			XV_X, x,
			XV_Y, y,
			XV_HELP_DATA, "cm:TodoHelp",
			NULL);
        	tdi->box_pi = xv_create(t->panel, PANEL_CHECK_BOX,
               		XV_X, x + 33,
                	XV_Y, y - 6,
                	PANEL_VALUE, (a->appt_status == completed ? 1 : 0),
			PANEL_NOTIFY_PROC, t_check_box,
			PANEL_CLIENT_DATA, tdi,
			XV_HELP_DATA, "cm:TodoHelp",
                	NULL);
		tdi->pi = xv_create(t->panel, PANEL_MESSAGE,
			PANEL_LABEL_STRING, buf3,
			XV_X, x + 60,
			XV_Y, y,
			XV_HELP_DATA, "cm:TodoHelp",
			NULL);
		
		tdi->next = NULL;
		if (t->head_todo == NULL) 
			t->head_todo = tdi;
		if (t->last_todo != NULL)
			t->last_todo->next = tdi;
		t->last_todo = tdi;
	}
	else {
		xv_set(ptr->box_pi, PANEL_VALUE, 
			(a->appt_status == completed ? 1 : 0), 
			PANEL_CLIENT_DATA, ptr,
			 NULL);
		xv_set(ptr->pi, PANEL_LABEL_STRING, buf3, NULL);
		ptr->uid.appt_id.tick = a->appt_id.tick;
		ptr->uid.appt_id.key = a->appt_id.key;
		ptr->uid.next = NULL; 
		t->last_todo = ptr;
		ptr = ptr->next;
	}

	return ptr;

}
struct Todo_item *
t_do_todo(c, range, ptr, x, y, item_num)
	Calendar *c;
	Range range;
	struct Todo_item *ptr;
	int x, *y, *item_num;
{
	Todo *t = (Todo*)c->todo;
	Abb_Appt *a = NULL, *list = NULL;

	(void)table_abbrev_lookup_range(c->view->current_calendar, &range, &list);
	if (list == NULL) 
		return ptr;

	/* reuse panel items if possible */
	for (a = list; a != NULL; a = a->next) {
		if (a->tag->tag == toDo) { 
			ptr = t_create_todo_item(ptr, t, x, *y, a, *item_num);
			*y += TODO_GAP;
			(*item_num)++;
		}
        }
	destroy_abbrev_appt(list);

	return ptr;
}

static caddr_t
make_todo(c)
	Calendar *c;
{
	static Todo    *t;

	t = (Todo*) c->todo;
	t->head_todo = t->last_todo = NULL;
	t->num_items = 0;
	t->header[0] = '\0';

	t->frame = xv_create(c->frame, FRAME_CMD,
		FRAME_INHERIT_COLORS, TRUE,
                FRAME_SHOW_LABEL, TRUE,
                FRAME_CMD_PUSHPIN_IN, TRUE,
                FRAME_SHOW_FOOTER, TRUE,
                FRAME_SHOW_RESIZE_CORNER, TRUE,
                FRAME_DONE_PROC, todo_done_proc,
		WIN_CMS,     (Cms)xv_get(c->frame, WIN_CMS),
                WIN_CLIENT_DATA, c,
                XV_WIDTH, 350,
                XV_HEIGHT, MIN_HEIGHT,
		XV_HELP_DATA, "cm:TodoHelp",
                0);
	t->panel = xv_create(t->frame, SCROLLABLE_PANEL,
                XV_X, 0,
                XV_Y, 1,
                WIN_CLIENT_DATA, c,
		XV_HELP_DATA, "cm:TodoHelp",
                0);
        t->sb = xv_create(t->panel, SCROLLBAR, 
		SCROLLBAR_DIRECTION, SCROLLBAR_VERTICAL,
                SCROLLBAR_PIXELS_PER_UNIT, TODO_GAP,
                SCROLLBAR_VIEW_LENGTH, MIN_HEIGHT / TODO_GAP,
                SCROLLBAR_PAGE_LENGTH, 3,
                SCROLLBAR_OBJECT_LENGTH,  (MIN_HEIGHT/TODO_GAP),
		NULL);

	notify_interpose_event_func(t->frame, t_event, NOTIFY_SAFE);

	ds_position_popup(c->frame, t->frame, DS_POPUP_LOR);

	c->todo = (caddr_t)t;

        xv_set(t->frame, XV_SHOW, TRUE, NULL);

	return (caddr_t)t;
}

extern void
t_create_todolist(c, glance)
	Calendar *c;
	Glance glance;
{
	Todo *t = (Todo*)c->todo;
	Props *p = (Props*)c->properties;
	Range range, oldrange;
	int x, y, item_num = 1;
	int new_glance = FALSE;
	char 	buf[128], buf2[160];
	struct Todo_item *ptr=NULL;

	buf[0] = buf2[0] = '\0'; 
        range.next = NULL;
	x = c->view->outside_margin*2;
	y = 10;
	if (t->none_pi != NULL)
		xv_set(t->none_pi, XV_SHOW, FALSE, NULL);
	if (t->glance != glance)
		new_glance = TRUE;
	t->glance = glance;
	switch(glance) {
		case yearGlance:
                        oldrange.key1 = lowerbound(jan1(c->view->olddate));
                        oldrange.key2 = upperbound(nextyear(oldrange.key1));
			range.key1 = lowerbound(jan1(c->view->date));
        		range.key2 = upperbound(nextyear(range.key1));
                        if (t->changed || t->head_todo == NULL || new_glance ||
				(oldrange.key1 != range.key1) && (oldrange.key2 != range.key2)) {
				sprintf(buf,  MGET("To Do List: Year of %d"), 
					year(range.key1+1));
				xv_set(t->panel, XV_SHOW, FALSE, NULL);
        			xv_set(t->frame, FRAME_BUSY, TRUE, NULL);
				ptr = t_do_todo(c, range, t->head_todo, x, &y, 
					&item_num);
				xv_set(t->panel, XV_SHOW, TRUE, NULL);
        			xv_set(t->frame, FRAME_BUSY, FALSE, NULL);
			}
			else {
				t->changed = false;
				return;
			}
		break;
		case monthGlance:
                        oldrange.key1 = first_dom(c->view->olddate);
                        oldrange.key2 = last_dom(c->view->olddate);
			range.key1 = first_dom(c->view->date);
        		range.key2 = last_dom(c->view->date);
                        if (t->changed || t->head_todo == NULL || new_glance ||
				 (oldrange.key1 != range.key1) && (oldrange.key2 != range.key2)) {
				format_date(range.key1+1, p->ordering_VAL, buf2, 0);
				sprintf(buf,  MGET("To Do List: %s"), buf2);
				xv_set(t->panel, XV_SHOW, FALSE, NULL);
        			xv_set(t->frame, FRAME_BUSY, TRUE, NULL);
				ptr = t_do_todo(c, range, t->head_todo, x, &y, 
					&item_num);
				xv_set(t->panel, XV_SHOW, TRUE, NULL);
        			xv_set(t->frame, FRAME_BUSY, FALSE, NULL);
			}
			else {
				t->changed = false;
				return;
			}
		break;
		case weekGlance:
                        oldrange.key1 = first_dow(c->view->olddate);
                        oldrange.key2 = last_dow(c->view->olddate);
			range.key1 = first_dow(c->view->date);
        		range.key2 = last_dow(c->view->date);
                        if (t->changed || t->head_todo == NULL || new_glance ||
				 (oldrange.key1 != range.key1) && (oldrange.key2 != range.key2)) {
				format_date(range.key1+1, p->ordering_VAL, buf2, 1);
				sprintf(buf, MGET("To Do List: Week of %s"), buf2);
				xv_set(t->panel, XV_SHOW, FALSE, NULL);
        			xv_set(t->frame, FRAME_BUSY, TRUE, NULL);
				ptr = t_do_todo(c, range, t->head_todo, x, &y, 
					&item_num);
				xv_set(t->panel, XV_SHOW, TRUE, NULL);
        			xv_set(t->frame, FRAME_BUSY, FALSE, NULL);
			}
			else {
				t->changed = false;
				return;
			}
		break;
		case dayGlance:
                        oldrange.key1 = lowerbound(c->view->olddate);
                        oldrange.key2 = upperbound(c->view->olddate);
			range.key1 = lowerbound(c->view->date);
        		range.key2 = upperbound(c->view->date);
			if (t->changed || t->head_todo == NULL || new_glance ||
				 (oldrange.key1 != range.key1) && (oldrange.key2 != range.key2)) {
				format_date(range.key1+1, p->ordering_VAL, buf2, 1);
				sprintf(buf,  MGET("To Do List: %s") , buf2);
				xv_set(t->panel, XV_SHOW, FALSE, NULL);
        			xv_set(t->frame, FRAME_BUSY, TRUE, NULL);
				ptr = t_do_todo(c, range, t->head_todo, x, &y, 
					&item_num);
				xv_set(t->panel, XV_SHOW, TRUE, NULL);
        			xv_set(t->frame, FRAME_BUSY, FALSE, NULL);
			}
			else {
				t->changed = false;
				return;
			}
		break;
	}
	t->changed = false;

	if (ptr != NULL) {
		if (ptr == t->head_todo) 
			t->head_todo = t->last_todo = NULL;
		else 
			t->last_todo->next = NULL;
		t_destroy_all(ptr);
	}
	t->num_items = (y-10) / TODO_GAP;
	if (t->num_items == 0)
		if (t->none_pi == NULL)
			t->none_pi = xv_create(t->panel, PANEL_MESSAGE,
				PANEL_LABEL_STRING,  MGET("No To Do Items") ,
				PANEL_LABEL_BOLD, TRUE,
				XV_X,  x,
				XV_Y,  y,
				XV_HELP_DATA, "cm:TodoHelp",
				NULL);
		else 
			xv_set(t->none_pi, XV_SHOW, TRUE, NULL);
	else if (t->none_pi != NULL)
		xv_set(t->none_pi, XV_SHOW, FALSE, NULL);

	xv_set(t->sb, SCROLLBAR_OBJECT_LENGTH,
                (y-10) / TODO_GAP,
		SCROLLBAR_VIEW_START, 0, NULL);

        xv_set(t->frame, XV_LABEL, buf, NULL);
}
extern void
cm_show_todo(m, mi)
	Menu m;
        Menu_item mi;
{
        Calendar *c;
        Todo *t;

        c = (Calendar *) xv_get(m, MENU_CLIENT_DATA, NULL);
        t = (Todo *) c->todo;
        if (t == NULL || t->frame == NULL)
                t = (Todo*)make_todo(c);
	else 
		xv_set(t->frame, XV_SHOW, true, NULL);

	t_create_todolist(c, (Glance)xv_get(mi, MENU_VALUE));
}
static void
t_set_pagenum(t)
	Todo *t;
{
	int x;

	x = t->num_items;
	if (x > MAX_TODO_LP) {
		x -= MAX_TODO_LP+1;
		t->num_pages = (x / MAX_TODO)+2;
	}
	else 
		t->num_pages = 1;
		 
}
static void
t_count_items(a, t, meoval, appt_type)
	Abb_Appt *a;
        Todo *t;
	int meoval;
	Event_Type appt_type;
{
	while (a != NULL) {
		switch(a->privacy) {
                        case public:
			if ((meoval == 2) || (meoval == 4)
				 || (meoval == 6)) {
                                        a=a->next;
                                        continue;
                                }
                                break;
                        case semiprivate:
			if ((meoval == 1) || (meoval == 4) 
				|| (meoval == 5)) {
                                        a=a->next;
                                        continue;
                                }
                                break;
                        case private:
			if ((meoval == 1) || (meoval == 2) 
				|| (meoval == 3)) {
                                        a=a->next;
                                        continue;
                                }
                                break;
                        default:
                                break;
                }
		if (appt_type == toDo && a->tag->tag == toDo)
			t->num_items++;
		else if (appt_type == appointment && 
				a->tag->tag == appointment)
			t->num_items++;
		a = a->next;
	}
}
extern void
print_list(c, appt_type, glance)
	Calendar *c;
	Event_Type appt_type;
	Glance glance;
{
	FILE *fp;
        Todo *t;
        Props *p = (Props *) c->properties;
	Range range;
	int i=0, meoval;
	char buf[80];
	Abb_Appt *a = NULL, *a_ptr = NULL, *prev_a = NULL, *appt_list = NULL;
	Props *pr = (Props*)c->properties;
	
	if ((fp = ps_open_file()) == NULL)
                return;

        if (c->todo == NULL)
		c->todo = (caddr_t) ckalloc(sizeof(Todo));

	t = (Todo*) c->todo;

	ps_init_printer(fp, PORTRAIT);
	ps_init_list(fp);

	range.next = NULL;
	t->num_items = 0;
	t->curr_page = 1;
	buf[0] = '\0';
	meoval = pr->meoVAL;
	switch(glance) {
		case yearGlance:
			/* count items and set up the year list */
			range.key1 = lowerbound(jan1(c->view->date));
			range.key2 = lowerbound(nextjan1(range.key1+1))+1;
			table_abbrev_lookup_range(calendar->view->current_calendar, 
				&range, &appt_list);
			if (a == NULL) a = appt_list;
			if (appt_list != NULL) {
				t_count_items(appt_list, t, meoval, appt_type);
				if (prev_a != NULL)
					prev_a->next = appt_list;
				for (a_ptr = appt_list; a_ptr->next != NULL;
					a_ptr=a_ptr->next);
				prev_a = a_ptr;
			}
			t_set_pagenum(t);
			if (appt_type == toDo)
				sprintf(t->header, MGET("To Do List: Year of %d"),
					year(range.key1+1));
			else
				sprintf(t->header, MGET("Appt List: Year of %d"), 
					year(range.key1+1));
			ps_print_header(fp, t->header);
			(void)ps_todo_outline(fp, t, appt_type);
			(void)ps_print_todo(fp, a, appt_type, yearGlance);
		break;
		case monthGlance:
			range.key1 = first_dom(c->view->date);
        		range.key2 = last_dom(c->view->date)+1;
			format_date(range.key1+1, p->ordering_VAL, buf, 0);
			if (appt_type == toDo)
				sprintf(t->header,  MGET("To Do List: %s"), buf);
			else
				sprintf(t->header,  MGET("Appt List: %s"), buf);
			ps_print_header(fp, t->header);
			table_abbrev_lookup_range(
				calendar->view->current_calendar, 
				&range, &a);
			t_count_items(a, t, meoval, appt_type);
			t_set_pagenum(t);
			ps_todo_outline(fp, t, appt_type);
			ps_print_todo(fp, a, appt_type, 
					monthGlance);
		break;
		case weekGlance:
			range.key1 = first_dow(c->view->date);
        		range.key2 = last_dow(c->view->date)+1;
			format_date(range.key1+1, p->ordering_VAL, buf, 1);
			if (appt_type == toDo)
				sprintf(t->header, MGET("To Do List: Week of %s"), buf);
			else
				sprintf(t->header, MGET("Appt List: Week of %s"), buf);
			ps_print_header(fp, t->header);
			table_abbrev_lookup_range(
				calendar->view->current_calendar, 
				&range, &a);
			t_count_items(a, t, meoval, appt_type);
			t_set_pagenum(t);
			ps_todo_outline(fp, t, appt_type);
			ps_print_todo(fp, a, appt_type, weekGlance);
		break;
		case dayGlance:
			range.key1 = lowerbound(c->view->date);
        		range.key2 = upperbound(c->view->date)+1;
			format_date(range.key1+1, p->ordering_VAL, buf,1);
			if (appt_type == toDo)
				sprintf(t->header,  MGET("To Do List: %s"), buf);
			else
				sprintf(t->header,  MGET("Appt List: %s"), buf);
			ps_print_header(fp, t->header);
			table_abbrev_lookup_range(
				calendar->view->current_calendar, 
				&range, &a);
			t_count_items(a, t, meoval, appt_type);
			t_set_pagenum(t);
			ps_todo_outline(fp, t, appt_type);
			ps_print_todo(fp, a, appt_type, dayGlance);
		break;
	}

	destroy_abbrev_appt(a);
	ps_finish_printer (fp);
	fclose(fp);
	ps_print_file();
}
extern Notify_value
ps_todo_button (m, mi)
    Menu m;
    Menu_item mi;
{
        Calendar *c = (Calendar *) xv_get(m, MENU_CLIENT_DATA, 0);

	print_list(c, toDo, (Glance)xv_get(mi, MENU_VALUE));
}
