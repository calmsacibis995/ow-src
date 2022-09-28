#ifndef lint
static  char sccsid[] = "@(#)alist.c 3.10 93/01/25 Copyr 1991 Sun Microsystems, Inc.";
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
#include "graphics.h"
#include "datefield.h"
#include "props.h"
#include "ps_graphics.h"
#include "calendar.h"
#include "alist.h"
#include "todo.h"
#include "ds_popup.h"
#include "table.h"
#include "gettext.h"

#define ALIST_GAP 25
#define ALIST_TOP 10
#define MIN_HEIGHT 330

extern Boolean
alist_showing(a)
        Alist *a;
{
        if (a != NULL && a->frame != NULL && xv_get(a->frame, XV_SHOW))
                return true;
        return false;
}
alist_destroy_all(al)
        struct Alist_item *al;
{
	if (al == NULL) return;
	alist_destroy_all(al->next);
	if (al != NULL) {
		/* must set pi's to show false cuz toolkit doesnt and
			display gets garbled */
		destroy_abbrev_appt(al->a);
		xv_set(al->pi, XV_SHOW, FALSE, NULL);
		xv_destroy_safe(al->pi);
		xv_set(al->numpi, XV_SHOW, FALSE, NULL);
		xv_destroy_safe(al->numpi);
	}
	free(al);
	al = NULL;
}

extern Notify_value
alist_done_proc(frame)
	Frame frame;
{
        Calendar *c;
	Alist *al;
	struct Alist_item *ali = NULL, *prev_ali = NULL;
 
	if (frame == NULL) return NOTIFY_DONE;
	c = (Calendar*)xv_get(frame, WIN_CLIENT_DATA);
	al = (Alist*)c->alist;

	for(prev_ali = ali = al->head_alist; ali != NULL; prev_ali = ali) {
		destroy_abbrev_appt(ali->a);
		ali = ali->next;
		free(prev_ali);
	}

	xv_destroy_safe(al->frame);
	al->frame = NULL;
	al->head_alist = NULL;
	al->last_alist = NULL;
	al->none_pi = NULL;
        return NOTIFY_DONE;
}
static Notify_value
a_event(client, event, arg, when)
        Notify_client     client;
        Event             *event;
        Notify_arg        arg;
        Notify_event_type when;
{
	Calendar *c;
	Alist *a;

	if (event_action(event) == WIN_RESIZE)
        {
		c = (Calendar*)xv_get(client, WIN_CLIENT_DATA);
		a = (Alist*)c->alist;
		xv_set(a->sb, SCROLLBAR_OBJECT_LENGTH, 
				a->num_items,
				SCROLLBAR_VIEW_LENGTH,
			 xv_get(a->frame, XV_HEIGHT) / ALIST_GAP,
		NULL);
		
	}
	return (notify_next_event_func(client, (Notify_event)event, arg, when));
}
struct Alist_item *
a_create_alist_item(ptr, alist, x, y, a, item_num)
	struct Alist_item *ptr;
        Alist *alist;
	int x, y;
	Abb_Appt *a;
	int item_num;
{
	Calendar *c = calendar;
	Props *p = (Props*)c->properties;
	struct Alist_item  *ali;
	Lines *lines = NULL;
	char	buf[105], buf2[115], buf3[10];
	char num_items[20];

	buf[0] = buf2[0] = buf3[0] = '\0';
	lines = text_to_lines(a->what, 1);

	format_maxchars(a, buf, 100, p->default_disp_VAL);
	if (item_num > 9)
		sprintf(num_items, "%d.", item_num);
	else
		sprintf(num_items, " %d.", item_num);
	if (alist->glance != dayGlance) {
		format_date3(a->appt_id.tick, p->ordering_VAL, 
			p->separator_VAL, buf3);
		sprintf(buf2, "%s  %s", buf3, buf);
	}
	else
		cm_strcpy(buf2, buf);
		
	if (ptr == NULL) {
		ali = (struct Alist_item*)ckalloc(sizeof(struct Alist_item));
		ali->a = (Abb_Appt *)copy_single_abbrev_appt(a);
		ali->numpi = xv_create(alist->panel, PANEL_MESSAGE,
                        PANEL_LABEL_STRING, num_items,
                        PANEL_LABEL_BOLD, TRUE,
                        XV_X, x,
                        XV_Y, y,
                        XV_HELP_DATA, "cm:AlistHelp",
                        NULL);

		ali->pi = xv_create(alist->panel, PANEL_MESSAGE,
			PANEL_LABEL_STRING, buf2,
			XV_X, x + 30,
			XV_Y, y,
			XV_HELP_DATA, "cm:AlistHelp",
			NULL);
		
		ali->next = NULL;
		if (alist->head_alist == NULL) 
			alist->head_alist = ali;
		if (alist->last_alist != NULL)
			alist->last_alist->next = ali;
		alist->last_alist = ali;
	}
	else {
		xv_set(ptr->pi, PANEL_LABEL_STRING, buf2, NULL);
		xv_set(ptr->numpi, PANEL_LABEL_STRING, num_items, NULL);
		destroy_abbrev_appt(ptr->a);
		ptr->a = (Abb_Appt *)copy_single_abbrev_appt(a);
		alist->last_alist = ptr;
		ptr = ptr->next;
	}

	free(lines);
	return ptr;

}
struct Alist_item *
a_do_alist(c, range, ptr, x, y, item_num)
	Calendar *c;
	Range range;
	struct Alist_item *ptr;
	int x, *y, *item_num;
{
	Alist *al = (Alist*)c->alist;
	Abb_Appt *a = NULL, *list = NULL;

	(void)table_abbrev_lookup_range(c->view->current_calendar, 
					&range, &list);
	if (list == NULL) 
		return ptr;

	/* reuse panel items if possible */
	for (a = list; a != NULL; a = a->next) {
		if (a->tag->tag == appointment) {
			ptr = a_create_alist_item(ptr, al, x, *y, a, 
					*item_num);
			*y += ALIST_GAP;
			(*item_num)++;
		}
        }
	destroy_abbrev_appt(list);

	return ptr;
}

extern caddr_t
make_alist(c)
	Calendar *c;
{
	static Alist    *a;

	c->alist = (caddr_t) ckalloc(sizeof(Alist));
	a = (Alist*) c->alist;
	a->head_alist = a->last_alist = NULL;
	a->num_items = 0;
	a->header[0] = '\0';

	a->frame = xv_create(c->frame, FRAME_CMD,
		FRAME_INHERIT_COLORS, TRUE,
                FRAME_SHOW_LABEL, TRUE,
                FRAME_CMD_PUSHPIN_IN, TRUE,
                FRAME_SHOW_FOOTER, TRUE,
                FRAME_SHOW_RESIZE_CORNER, TRUE,
                FRAME_DONE_PROC, alist_done_proc,
		WIN_CMS,     (Cms)xv_get(c->frame, WIN_CMS),
                WIN_CLIENT_DATA, c,
                XV_WIDTH, 350,
                XV_HEIGHT, MIN_HEIGHT,
		XV_HELP_DATA, "cm:AlistHelp",
                0);
	a->panel = xv_create(a->frame, SCROLLABLE_PANEL,
                XV_X, 0,
                XV_Y, 1,
                WIN_CLIENT_DATA, c,
		XV_HELP_DATA, "cm:AlistHelp",
                0);
        a->sb = xv_create(a->panel, SCROLLBAR, 
		SCROLLBAR_DIRECTION, SCROLLBAR_VERTICAL,
                SCROLLBAR_PIXELS_PER_UNIT, ALIST_GAP,
                SCROLLBAR_VIEW_LENGTH, MIN_HEIGHT / ALIST_GAP,
                SCROLLBAR_PAGE_LENGTH, 3,
                SCROLLBAR_OBJECT_LENGTH,  (MIN_HEIGHT/ALIST_GAP),
		NULL);

	notify_interpose_event_func(a->frame, a_event, NOTIFY_SAFE);

	ds_position_popup(c->frame, a->frame, DS_POPUP_LOR);

	c->alist = (caddr_t)a;

        xv_set(a->frame, XV_SHOW, TRUE, NULL);

	return (caddr_t)a;
}

extern void
a_create_alist(c, glance)
	Calendar *c;
	Glance glance;
{
	Alist *a = (Alist*)c->alist;
	Props *p = (Props*)c->properties;
	Range range, oldrange;
	int x, y, item_num = 1;
	int new_glance = FALSE;
	char 	buf[80], buf2[80];
	struct Alist_item *ptr=NULL;

	buf[0] = buf2[0] = '\0'; 
        range.next = NULL;
	x = c->view->outside_margin*2;
	y = ALIST_TOP;
	if (a->none_pi != NULL)
		xv_set(a->none_pi, XV_SHOW, FALSE, NULL);
	if (a->glance != glance)
		new_glance = TRUE;
	a->glance = glance;
	
	switch(glance) {
		case yearGlance:
			oldrange.key1 = lowerbound(jan1(c->view->olddate));
        		oldrange.key2 = upperbound(nextyear(oldrange.key1));
			range.key1 = lowerbound(jan1(c->view->date));
        		range.key2 = upperbound(nextyear(range.key1));
			/* STRING_EXTRACTION SUNW_DESKSET_CM_MSG :
			 *
			 * Appt = Appointment
			 */
			if (a->changed || a->head_alist == NULL || new_glance || (oldrange.key1 != range.key1) && (oldrange.key2 != range.key2)) {
				sprintf(buf, MGET("Appt List: Year of %d"), year(range.key1+1));
				ptr = a->head_alist;
				xv_set(a->panel, XV_SHOW, FALSE, NULL);
        			xv_set(a->frame, FRAME_BUSY, TRUE, NULL);
				ptr = a_do_alist(c, range, ptr, x, &y, &item_num);
				xv_set(a->panel, XV_SHOW, TRUE, NULL);
        			xv_set(a->frame, FRAME_BUSY, FALSE, NULL);
			}
			else {	
				a->changed = false;
				return;
			}
		break;
		case monthGlance:
			oldrange.key1 = first_dom(c->view->olddate);
        		oldrange.key2 = last_dom(c->view->olddate);
			range.key1 = first_dom(c->view->date);
        		range.key2 = last_dom(c->view->date);
			if (a->changed || a->head_alist == NULL || new_glance || (oldrange.key1 != range.key1) && (oldrange.key2 != range.key2)) {
				format_date(range.key1+1, p->ordering_VAL, buf2, 0);
				sprintf(buf, MGET("Appt List: %s"), buf2);
				xv_set(a->panel, XV_SHOW, FALSE, NULL);
        			xv_set(a->frame, FRAME_BUSY, TRUE, NULL);
				ptr = a_do_alist(c, range, a->head_alist, x, &y, &item_num);
				xv_set(a->panel, XV_SHOW, TRUE, NULL); 
                                xv_set(a->frame, FRAME_BUSY, FALSE, NULL); 
			}
			else {	
				a->changed = false;
				return;
			}
		break;
		case weekGlance:
			oldrange.key1 = first_dow(c->view->olddate);
        		oldrange.key2 = last_dow(c->view->olddate);
			range.key1 = first_dow(c->view->date);
        		range.key2 = last_dow(c->view->date);
			if (a->changed || a->head_alist == NULL || new_glance || (oldrange.key1 != range.key1) && (oldrange.key2 != range.key2)) {
				format_date(range.key1+1, p->ordering_VAL, buf2, 1);
				sprintf(buf, MGET("Appt List: Week of %s"), buf2);
				xv_set(a->panel, XV_SHOW, FALSE, NULL);
        			xv_set(a->frame, FRAME_BUSY, TRUE, NULL);
				ptr = a_do_alist(c, range, a->head_alist, x, &y, &item_num);
				xv_set(a->panel, XV_SHOW, TRUE, NULL); 
                                xv_set(a->frame, FRAME_BUSY, FALSE, NULL); 
			}
			else {	
				a->changed = false;
				return;
			}
		break;
		case dayGlance:
			oldrange.key1 = lowerbound(c->view->olddate);
        		oldrange.key2 = upperbound(c->view->olddate);
			range.key1 = lowerbound(c->view->date);
        		range.key2 = upperbound(c->view->date);
			if (a->changed || a->head_alist == NULL || new_glance || (oldrange.key1 != range.key1) && (oldrange.key2 != range.key2)) {
				format_date(range.key1+1, p->ordering_VAL, buf2, 1);
				sprintf(buf, MGET("Appt List: %s"), buf2);
				xv_set(a->panel, XV_SHOW, FALSE, NULL);
        			xv_set(a->frame, FRAME_BUSY, TRUE, NULL);
				ptr = a_do_alist(c, range, a->head_alist, x, &y, &item_num);
				xv_set(a->panel, XV_SHOW, TRUE, NULL); 
                                xv_set(a->frame, FRAME_BUSY, FALSE, NULL); 
			}
			else {	
				a->changed = false;
				return;
			}
		break;
	}
	a->changed = false;

	/* This means there are some left over items to destroy and */
	/* 'ptr' prints to the remaining list that we need to destroy. */
	/* We reuse the panel items. */
	if (ptr != NULL) {
		alist_destroy_all(ptr);
		/* if ptr points to head then no items left on stack */
		if (ptr == a->head_alist) 
			a->head_alist = a->last_alist = NULL;
		/* otherwise there are some */
		else 
			a->last_alist->next = NULL;
	}
	a->num_items = (y-ALIST_TOP) / ALIST_GAP;
	if (a->num_items == 0) {
		if (a->none_pi == NULL)
			a->none_pi = xv_create(a->panel, 
				PANEL_MESSAGE,
				PANEL_LABEL_STRING, LGET("No Appointments"),
				PANEL_LABEL_BOLD, TRUE,
				XV_X,  x,
				XV_Y,  y,
				XV_HELP_DATA, "cm:AlistHelp",
				NULL);
		else
			xv_set(a->none_pi, XV_SHOW, TRUE, NULL);
	}
	else if (a->none_pi != NULL)
                xv_set(a->none_pi, XV_SHOW, FALSE, NULL);

	xv_set(a->sb, SCROLLBAR_OBJECT_LENGTH,
                (y-ALIST_TOP) / ALIST_GAP,
		SCROLLBAR_VIEW_START, 0, NULL);
	xv_set(a->frame, XV_LABEL, buf, NULL);
}
extern void
cm_show_appts(m, mi)
	Menu m;
        Menu_item mi;
{
        Calendar *c;
        Alist *a;

        c = (Calendar *) xv_get(m, MENU_CLIENT_DATA);
        a = (Alist *) c->alist;
        if (a == NULL || a->frame == NULL)
                c->alist = (caddr_t)make_alist(c);
	else
		xv_set(a->frame, XV_SHOW, true, NULL);

	a_create_alist(c, (Glance)xv_get(mi, MENU_VALUE));

}
extern Notify_value
ps_appt_button (m, mi)
    Menu m;
    Menu_item mi;
{
        Calendar *c = (Calendar *) xv_get(m, MENU_CLIENT_DATA);

        print_list(c, appointment, (Glance)xv_get(mi, MENU_VALUE));
}
