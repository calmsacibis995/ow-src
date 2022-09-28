#ifndef lint
static  char sccsid[] = "@(#)blist.c 1.38 94/05/16 Copyr 1991 Sun Microsystems, Inc.";
#endif
/*
 *  Copyright (c) 1987-1990 Sun Microsystems, Inc.
 *  All Rights Reserved.
 *
 *  Sun considers its source code as an unpublished, proprietary
 *  trade secret, and it is available only under strict license
 *  provisions.  This copyright notice is placed here only to protect
 *  Sun in the event the source is deemed a published work.  Dissassembly, *  decompilation, or other means of reducing the object code to human
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
#include <rpc/rpc.h>
#include <xview/xview.h>
#include <xview/xv_xrect.h>
#include <xview/cms.h>
#include <xview/frame.h>
#include <xview/font.h>
#include <xview/panel.h>
#include <xview/notice.h>
#include <ds_listbx.h>
#include "appt.h"
#include "util.h"
#include "timeops.h"
#include "datefield.h"
#include "props.h"
#include "graphics.h"
#include "browser.h"
#include "common.h"
#include "blist.h"
#include "calendar.h"
#include "gettext.h"
#include "ds_popup.h"

static Boolean list_changed, first_cancel;
static BrowserState 	*state_list;

extern Boolean
browselist_exists(bl)
        Browselist *bl;
{
        if (bl != NULL && bl->frame != NULL)
                return true;
        return false;
}
extern Boolean
browselist_showing(bl)
        Browselist *bl;
{
        if (bl != NULL && bl->frame != NULL && xv_get(bl->frame, XV_SHOW))
                return true;
        return false;
}

/* ARGSUSED */
static void
blist_check_changebutton(bl)
        Browselist *bl;
{
 
        if (list_num_selected(bl->list) > 1)
                xv_set(bl->changebutton, PANEL_INACTIVE, TRUE, NULL);
        else
                xv_set(bl->changebutton, PANEL_INACTIVE, FALSE, NULL);
}

/* ARGSUSED */
static void
blist_select_all(menu, mi)
        Menu    menu;
        Menu_item mi;
{
	Calendar *c = (Calendar*)xv_get(menu, MENU_CLIENT_DATA);
	Browselist *b = (Browselist*)c->browselist;

	xv_set(b->list, XV_SHOW, false, NULL);
	list_select_all(b->list);
	xv_set(b->list, XV_SHOW, true, NULL);
	blist_check_changebutton(b);
}
/* ARGSUSED */
static void
blist_deselect_all(menu, mi)
        Menu    menu;
        Menu_item mi;
{
	Calendar *c = (Calendar*)xv_get(menu, MENU_CLIENT_DATA);
	Browselist *b = (Browselist*)c->browselist;

	xv_set(b->list, XV_SHOW, false, NULL);
	list_deselect_all(b->list);
	xv_set(b->list, XV_SHOW, true, NULL);
	blist_check_changebutton(b);
}
static Menu
make_blist_menu(c)
        Calendar *c;
{
        Menu box_menu;

        box_menu = menu_create(
                MENU_TITLE_ITEM,  LGET("Browse List") ,
                MENU_CLIENT_DATA,       c,
                MENU_NOTIFY_STATUS, XV_ERROR,
                MENU_ITEM,
                        MENU_STRING,  LGET("Select All") ,
                        MENU_ACTION_PROC, blist_select_all,
                        MENU_CLIENT_DATA,       c,
                        0,
                MENU_ITEM,
                        MENU_STRING,  LGET("Deselect All") ,
                        MENU_ACTION_PROC, blist_deselect_all,
                        MENU_CLIENT_DATA,       c,
                        0,
                NULL);
	return box_menu;
}

static Notify_value
blist_add_name(item, event)
        Panel_item item;
        Event *event;
{
        Calendar        *c=NULL;
        Browselist    	*bl=NULL;
        Props    	*p=NULL;
        char            *val=NULL;

        c = (Calendar *) xv_get(item, PANEL_CLIENT_DATA);
        bl = (Browselist *) c->browselist;
        p = (Props *) c->properties;

        xv_set(bl->frame, FRAME_LEFT_FOOTER, "", NULL);
        val = (char *) xv_get(bl->username, PANEL_VALUE);
	if (blank_buf(val)) {
                (void) notice_prompt(bl->frame, (Event *)NULL,
                        NOTICE_MESSAGE_STRINGS,
                        MGET("Remove blanks from Name\nand Add Again."),
                        0,
                        NOTICE_BUTTON_YES,  LGET("Continue") ,
                        NULL);
                return NOTIFY_DONE;
        }

        if (val != NULL && *val != '\0') {
                if (list_dup(bl->list, val)) {
                        (void) notice_prompt(bl->frame, (Event *)NULL,
                        NOTICE_MESSAGE_STRINGS,
                        MGET("Name is Already in List"),
                        0,
                        NOTICE_BUTTON_YES, LGET("Continue"),
                        NULL);
                        return(NOTIFY_DONE);
                }

		if (strcmp(p->defcal_VAL, c->calname) != 0) 
        		list_add_entry(bl->list, val, NULL, NULL, 2, FALSE);
		else
        		list_add_entry(bl->list, val, NULL, NULL, 1, FALSE);

		first_cancel = list_changed = TRUE;
	}
	else 
		notice_prompt(bl->frame, (Event *)NULL,
			NOTICE_MESSAGE_STRINGS,
			MGET("User Name Not Specified."),
			0,
			NOTICE_BUTTON_YES, LGET("Continue"),
			NULL);

        return(NOTIFY_DONE);
}

static Notify_value
blist_change_name(item, event)
	Panel_item item;
	Event *event;
{
	Calendar *c;
	Browselist *bl;
	int i = -1;
	char *cal_name, *new_name;

	c = (Calendar *)xv_get(item, PANEL_CLIENT_DATA);
	bl = (Browselist *)c->browselist;

        xv_set(bl->frame, FRAME_LEFT_FOOTER, "", NULL);
	i = (int) xv_get(bl->list, PANEL_LIST_FIRST_SELECTED);
	if (i == -1) {
                notice_prompt(c->frame, (Event *)NULL,
                NOTICE_MESSAGE_STRINGS,
                EGET("Select a Name to Change"),
                0,
                NOTICE_BUTTON_YES,  LGET("Continue"),
                0);
                return;
        }          
	else {
		cal_name = (char *)xv_get(bl->list, PANEL_LIST_STRING, i);
		if (self(cal_name)) {
			notice_prompt(bl->frame, (Event *)NULL,
				NOTICE_MESSAGE_STRINGS,  
				EGET("Use Properties from Edit Menu to Change Default or Initial View Calendar...") ,
				0,
				NOTICE_BUTTON_YES,  LGET("Continue") ,
				0);
		}
		else {
			list_delete_entry_n(bl->list, i);
			new_name = (char*)xv_get(bl->username, PANEL_VALUE);
        		list_add_entry(bl->list, new_name, NULL, NULL, i, FALSE);
			xv_set(bl->list, PANEL_LIST_SELECT, i, TRUE, NULL); 
		}
	}
	first_cancel = list_changed = TRUE;
}

static Notify_value
blist_remove_name(item, event)
	Panel_item item;
	Event *event;
{
	Calendar *c;
	Browselist *bl;
	int i = -1;
	char *cal_name;

	c = (Calendar *)xv_get(item, PANEL_CLIENT_DATA);
	bl = (Browselist *)c->browselist;

        xv_set(bl->frame, FRAME_LEFT_FOOTER, "", NULL);
	i = (int) xv_get(bl->list, PANEL_LIST_FIRST_SELECTED);
	if (i == -1) {
                notice_prompt(c->frame, (Event *)NULL,
                NOTICE_MESSAGE_STRINGS,
                EGET("Select a Name to Remove"),
                0,
                NOTICE_BUTTON_YES,  LGET("Continue"),
                0);
                return;
        }          
	while (i != -1) {
		cal_name = (char *)xv_get(bl->list, PANEL_LIST_STRING, i);
		if (self(cal_name)) {
			notice_prompt(bl->frame, (Event *)NULL,
				NOTICE_MESSAGE_STRINGS,  
				EGET("Cannot Remove Default or Initial View Calendar...") ,
				0,
				NOTICE_BUTTON_YES,  LGET("Continue") ,
				0);
			xv_set(bl->list, PANEL_LIST_SELECT, i, FALSE, NULL);
		}
		i = xv_get(bl->list, PANEL_LIST_NEXT_SELECTED, i);
	}
	list_delete_selected(bl->list);

	first_cancel = list_changed = TRUE;

	blist_check_changebutton(bl);
}

static Notify_value
blist_show_cal(item, event)
	Panel_item item;
	Event *event;
{
	Calendar *c;
	Browselist *bl;
	int i = -1;
	char *name;

	c = (Calendar *)xv_get(item, PANEL_CLIENT_DATA);
	bl = (Browselist *)c->browselist;

	i = (int) xv_get(bl->list, PANEL_LIST_FIRST_SELECTED);

	if (i < 0) {
		/* browse on username instead */
		name = (char *)xv_get(bl->username, PANEL_VALUE);
		if (name == NULL || *name == '\0')
                	notice_prompt(c->frame, (Event *)NULL,
                        NOTICE_MESSAGE_STRINGS,
                        EGET("Select a calendar to browse"),
                        0,
                        NOTICE_BUTTON_YES,  LGET("Continue"),
                	0);
		else
			switch_it(name);
	}
	else {
		name = (char*)xv_get(bl->list, PANEL_LIST_STRING, i);
		switch_it(name);
	}
}

static void
init_blist(c)
        Calendar *c;
{
        char    *tmp, *name;
        char    *namelist;
        Browselist *bl = (Browselist*)c->browselist;
        Props *p = (Props*)c->properties;
        int position, hr, day_of_week;
 
	cal_update_props();
        tmp = cm_get_property(property_names[CP_DAYCALLIST]);
        namelist =  (char*)ckalloc(cm_strlen(tmp)+1);
        cm_strcpy(namelist, tmp);
        if (namelist != NULL && *namelist != NULL ) {
                name = (char *) strtok(namelist, " ");
                while (name != NULL) {
                        position = xv_get(bl->list, PANEL_LIST_NROWS);
                        list_add_entry(bl->list, name, (Pixrect*)NULL,
                                        NULL, position, FALSE);
                        name = (char *) strtok((char *)NULL, " ");
                }
        }
        if (!list_in_list(bl->list, c->calname, &position))
                list_add_entry(bl->list, c->calname, NULL, NULL, 0, FALSE);
        if (!list_in_list(bl->list, p->defcal_VAL, &position))
                list_add_entry(bl->list, p->defcal_VAL, NULL, NULL, 1, FALSE);
 
	blist_check_changebutton(bl);
        free (namelist);
	first_cancel = list_changed = FALSE;
}
static Notify_value
blist_done_proc(frame)
	Frame frame;
{
	int not_val;
	Calendar *c;
	Browselist *bl;

	c = (Calendar *) xv_get(frame, WIN_CLIENT_DATA);
	bl = (Browselist *) c->browselist;

	if (!first_cancel) {
		xv_set(bl->list, XV_SHOW, false, NULL);
		list_flush(bl->list);
		init_blist(c);
		xv_set(bl->list, XV_SHOW, true, NULL);
	}

	if ((!list_changed) || (!first_cancel)) {
        	xv_set(frame, XV_SHOW, FALSE, 0);
        	return(NOTIFY_DONE);
	}

        not_val = notice_prompt(frame, NULL,
                NOTICE_MESSAGE_STRINGS,
                MGET("You Have Made Changes That Have\nNot Been APPLIED. Do You Still\nWant To Dismiss Window?"),
                0,
                NOTICE_BUTTON_YES, LGET("Dismiss"),
                NOTICE_BUTTON_NO, LGET("Cancel"),
                0);

        if (not_val == NOTICE_YES) {
        	xv_set(frame, XV_SHOW, FALSE, 0);
		list_flush(bl->list);
		init_blist(c);
        }
        else {
 		xv_set(frame, FRAME_CMD_PUSHPIN_IN, TRUE,
                         XV_SHOW, TRUE, NULL);
		first_cancel = FALSE;
        }
        return NOTIFY_DONE;
}

static void
browser_save_state(list)
	Panel_item list;
{
	int i = -1;
	BrowserState *state = NULL, *head = NULL;

	i = xv_get(list, PANEL_LIST_FIRST_SELECTED);
	while (i != -1) {
		state = (BrowserState *) ckalloc(sizeof(BrowserState));
		state->cname = cm_strdup((char *)xv_get(list, PANEL_LIST_STRING, i));
                state->glyph = (Pixrect *)xv_get(list, PANEL_LIST_GLYPH, i);
		state->next = head;
		head = state;
		i = xv_get(list, PANEL_LIST_NEXT_SELECTED, i);
	}	
	state_list = head;
}

static BrowserState *
browser_find_match(name)
	char name[];
{
	BrowserState *ptr;

	ptr = state_list;

	while (ptr != NULL) {
		if (strcmp(name, ptr->cname) != 0)
			ptr = ptr->next;
		else
			return(ptr);
	}
	return(NULL);
}
static void
browser_retrieve_state(list)
	Panel_item list;
{
	int i, nrows;
	BrowserState *state, *match;

	nrows = xv_get(list, PANEL_LIST_NROWS);
	for (i = 0; i < nrows; i++) {
		match = browser_find_match((char *)xv_get(list, PANEL_LIST_STRING, i));
		if (!match)
			continue;
		xv_set(list, PANEL_LIST_SELECT, i, TRUE, NULL);
                if (match->glyph != NULL)
                        xv_set(list, PANEL_LIST_GLYPH, i, match->glyph, NULL);
	}
}

/* ARGSUSED */
static Notify_value
bl_apply_proc(item, event)
        Panel_item item;
        Event *event;
{
	Calendar *c;
	Browselist *bl;
	Browser *b;
	Props *p;

	if (!list_changed)
		return;
        c = (Calendar*)xv_get(item, PANEL_CLIENT_DATA);
        bl = (Browselist*)c->browselist;
        b = (Browser*)c->browser;
	p = (Props*)c->properties;

        xv_set(bl->frame, FRAME_LEFT_FOOTER, "", NULL);
	if (browser_exists(b)) {
		if (!browser_showing(b))
			list_copy_list(bl->list, b->box);
		else {
			browser_save_state(b->box);
                        xv_set(b->box, XV_SHOW, FALSE, NULL);
			list_copy_list(bl->list, b->box);
			browser_retrieve_state(b->box);
			browser_free_state();
			update_browser_display2(b, p);
                        xv_set(b->box, XV_SHOW, TRUE, NULL);
		}
	}
	blist_write_names(c, bl->list);
	list_changed = FALSE;
}

/* ARGSUSED */
static Notify_value
bl_reset_proc(item, event)
        Panel_item item;
        Event *event;
{
	Calendar *c;
	Browselist *bl;

	if (!list_changed)
		return;
        c = (Calendar*)xv_get(item, PANEL_CLIENT_DATA);
        bl = (Browselist*)c->browselist;

        xv_set(bl->frame, FRAME_LEFT_FOOTER, "", NULL);
	xv_set(bl->list, XV_SHOW, false, NULL);
	list_flush(bl->list);
	init_blist(c);
	xv_set(bl->list, XV_SHOW, true, NULL);
}

/* ARGSUSED */
static Notify_value
blist_select_notify(pi, string, cd, op, event, row)
        Panel_item pi;
        char *string;       	/* string of row being operated on */
        Xv_opaque cd;  		/* Client data of row */
        Panel_list_op op;   	/* operation */
        Event *event;
        int row;
{
	Calendar *c;
	Browselist *bl;

	c = (Calendar *) xv_get(pi, PANEL_CLIENT_DATA);
	bl = (Browselist *) c->browselist;

	if (op == PANEL_LIST_OP_SELECT)
		xv_set(bl->username, PANEL_VALUE, string, NULL);
	else if (op == PANEL_LIST_OP_DESELECT) {
		if (list_num_selected(bl->list) == 1) {
			row = (int)xv_get(bl->list, PANEL_LIST_FIRST_SELECTED);
			string = (char*)xv_get(bl->list, PANEL_LIST_STRING, row);
			xv_set(bl->username, PANEL_VALUE, string, NULL);
		}
	}
	blist_check_changebutton(bl);
}

/* ARGSUSED */
static Notify_value
blist_sort_alpha(item, event)
        Panel_item item;
        Event *event;
{
        Calendar *c;
        Browselist *bl;
        Props *p;
	Boolean default_cal = FALSE;
	int selection0 = 0, selection1 = 0;

        c = (Calendar*)xv_get(item, PANEL_CLIENT_DATA);
        p = (Props*)c->properties;
        bl = (Browselist*)c->browselist;

        xv_set(bl->frame, FRAME_LEFT_FOOTER, "", NULL);
        if (xv_get(bl->list, PANEL_LIST_NROWS) < 1) {
                notice_prompt(c->frame, (Event *)NULL,
                NOTICE_MESSAGE_STRINGS,
                EGET("There are no items to sort"),
                0,
                NOTICE_BUTTON_YES,  LGET("Continue"),
                0);
                return;
        }

        selection0 = (int)xv_get(bl->list, PANEL_LIST_SELECTED, 0);
        xv_set(bl->list, PANEL_LIST_DELETE, 0, NULL);
        if (strcmp(p->defcal_VAL, c->calname) != 0) {
		default_cal = TRUE;
                /* it's the zeroth item 'cause calname was removed */
                selection1 = (int)xv_get(bl->list, PANEL_LIST_SELECTED, 0);
        	xv_set(bl->list, PANEL_LIST_DELETE, 0, NULL);
	}
	xv_set(bl->list, PANEL_LIST_SORT, PANEL_FORWARD, NULL);
        list_add_entry(bl->list, c->calname, NULL, NULL, 0, FALSE);
        if (selection0)
                xv_set(bl->list, PANEL_LIST_SELECT, 0, TRUE, NULL);
	if (default_cal) {
        	list_add_entry(bl->list, p->defcal_VAL, NULL, NULL, 1, FALSE);
                if (selection1)
                        xv_set(bl->list, PANEL_LIST_SELECT, 1, TRUE, NULL); 
	}

	first_cancel = list_changed = TRUE;
}

extern void
make_blist(c)
	Calendar *c;
{
	Browselist *bl;
	int longest_str = 0, x_gap = 20, y_gap = 15, tmp, tmp2;
	int tmp3, tmp4, tmp5, tmp6, max;
	Panel_item addbutton, apply_button, reset_button, sortbutton, changebutton, removebutton;
	Panel panel;
	Font_string_dims dims;
	Xv_Font pf;
	char *t = NULL;

	bl = (Browselist*)  ckalloc(sizeof(Browselist));

	bl->frame = (Frame) xv_create(c->frame,  FRAME_CMD, 
		WIN_CMS,	(Cms)xv_get(c->frame, WIN_CMS),
		XV_HEIGHT, 311,
		XV_LABEL, LGET("CM: Setup Menu"),
		FRAME_SHOW_LABEL, TRUE,
                FRAME_CMD_PUSHPIN_IN, TRUE,
		FRAME_SHOW_FOOTER, TRUE,
		FRAME_DONE_PROC, blist_done_proc,
		WIN_CLIENT_DATA,  c,
		0);
	panel = xv_get(bl->frame, FRAME_CMD_PANEL);
	xv_set(panel, 
		XV_X, 0,
                XV_Y, 0,
                PANEL_CLIENT_DATA, c,
                XV_HELP_DATA, "cm:BlPanel",
                0);
	tmp = xv_col(panel, 1);
	tmp3 = xv_row(panel, 1);
        bl->username = xv_create(panel, PANEL_TEXT,
                XV_X, tmp,
                XV_Y, tmp3,
		PANEL_CLIENT_DATA, c,
                PANEL_VALUE_DISPLAY_LENGTH, 30,
                PANEL_VALUE_STORED_LENGTH, 80,
                PANEL_LAYOUT, PANEL_HORIZONTAL,
                PANEL_READ_ONLY, FALSE,
		PANEL_NOTIFY_PROC, blist_add_name,
                PANEL_LABEL_STRING, LGET("User Name:"),
		XV_HELP_DATA, "cm:BlUserName",
                NULL);
	tmp2 = xv_row(panel, 2);
	/* Borrowing the tmp4 & tmp5 variables since they are not used until later */
	/* Make sure the entire panel title is displayed when it is translated */
	tmp4 = xv_get(bl->username, XV_X) + xv_get(bl->username, XV_WIDTH) - x_gap;
	pf = (Xv_Font)xv_get(c->frame, XV_FONT);
	t = LGET("Browse Menu Items");
	(void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
	tmp5 = dims.width + 30;    /* 30 is approximately width of the scroll bar */
	if ( tmp5 > tmp4 ) {
		tmp4 = tmp5;
	}
        bl->list = xv_create(panel, PANEL_LIST,
                XV_X, tmp,
                XV_Y, tmp2 + y_gap,
		PANEL_CLIENT_DATA, c,
		PANEL_NOTIFY_PROC, blist_select_notify,
                PANEL_LIST_WIDTH, tmp4,
                PANEL_LIST_DISPLAY_ROWS, 7,
                PANEL_LIST_TITLE, t,
                PANEL_CHOOSE_ONE, FALSE,
                PANEL_CHOOSE_NONE, TRUE,
		PANEL_ITEM_MENU, (Menu)make_blist_menu(c),
		XV_HELP_DATA, "cm:BlList",
                NULL);
	tmp = xv_get(bl->list, XV_X) + xv_get(bl->list, XV_WIDTH) + x_gap;
        addbutton = xv_create(panel, PANEL_BUTTON,
                XV_X, tmp, 
                XV_Y, tmp3,
		PANEL_CLIENT_DATA, c,
		PANEL_NOTIFY_PROC, blist_add_name,
                PANEL_LABEL_STRING, LGET("Add Name"),
		XV_HELP_DATA, "cm:BlAdd",
                NULL);
	bl->changebutton = xv_create(panel, PANEL_BUTTON,
                XV_X, tmp, 
                XV_Y, xv_row(panel, 3) + y_gap,
		PANEL_NOTIFY_PROC, blist_change_name,
		PANEL_CLIENT_DATA, c,
                PANEL_LABEL_STRING, LGET("Change"),
		XV_HELP_DATA, "cm:BlChange",
                NULL);
	removebutton = xv_create(panel, PANEL_BUTTON,
                XV_X, tmp, 
                XV_Y, xv_row(panel, 4) + y_gap,
		PANEL_NOTIFY_PROC, blist_remove_name,
		PANEL_CLIENT_DATA, c,
                PANEL_LABEL_STRING, LGET("Remove"),
		XV_HELP_DATA, "cm:BlRemove",
                NULL);
	sortbutton = xv_create(panel, PANEL_BUTTON,
                XV_X, tmp, 
                XV_Y, xv_row(panel, 6) + y_gap,
		PANEL_NOTIFY_PROC, blist_sort_alpha,
		PANEL_CLIENT_DATA, c,
                PANEL_LABEL_STRING, LGET("Sort List"),
		XV_HELP_DATA, "cm:BlSort",
                NULL);
	tmp = xv_get(bl->list, XV_Y) + xv_get(bl->list, XV_HEIGHT) + 20;
        apply_button = xv_create(panel, PANEL_BUTTON,
                        PANEL_LABEL_STRING, LGET(" Apply "),
                        XV_Y, tmp,
                        PANEL_NOTIFY_PROC, bl_apply_proc,
                        PANEL_CLIENT_DATA, c,
                        XV_HELP_DATA, "cm:DefaultApply",
                        0);
        reset_button = xv_create(panel, PANEL_BUTTON,
                        PANEL_LABEL_STRING, LGET(" Reset "),
                        XV_Y, tmp,
                        PANEL_NOTIFY_PROC, bl_reset_proc,
                        PANEL_CLIENT_DATA, c,
                        XV_HELP_DATA, "cm:DefaultReset",
                        0);

        (void)xv_set(panel, PANEL_DEFAULT_ITEM, addbutton, 0);

	(void)xv_set(bl->frame, XV_LABEL, LGET("CM: Setup Menu"), 0);
	tmp2 = xv_get(addbutton, XV_WIDTH);
	tmp = xv_get(addbutton, XV_X) + tmp2;
        if (longest_str < tmp)
                longest_str = tmp;
	tmp3 = xv_get(removebutton, XV_WIDTH);
        tmp = xv_get(removebutton, XV_X) + tmp3;
        if (longest_str < tmp)
                longest_str = tmp;
	tmp4 = xv_get(sortbutton, XV_WIDTH);
        tmp = xv_get(sortbutton, XV_X) + tmp4;
        if (longest_str < tmp)
                longest_str = tmp;
	tmp5 = xv_get(bl->changebutton, XV_WIDTH);
        tmp = xv_get(bl->changebutton, XV_X) + tmp5;
        if (longest_str < tmp)
                longest_str = tmp;
	(void)xv_set(panel, XV_WIDTH, longest_str + 20,
			XV_HEIGHT, xv_get(apply_button, XV_Y) +
			xv_get(apply_button, XV_HEIGHT) + 15, NULL);
	(void)xv_set(bl->frame, WIN_FIT_WIDTH, 0,
			WIN_FIT_HEIGHT, 0, NULL);

	ds_center_items(panel, -1, apply_button, reset_button, NULL);

	max = tmp2;
        if (tmp3 > max)
                max = tmp3;
        if (tmp4 > max)
                max = tmp4;
        if (tmp5 > max)
                max = tmp5;

        xv_set(addbutton, PANEL_LABEL_WIDTH,
                (int)xv_get(addbutton, PANEL_LABEL_WIDTH) +
                max - tmp2,
                NULL);
        xv_set(removebutton, PANEL_LABEL_WIDTH,
                (int)xv_get(removebutton, PANEL_LABEL_WIDTH) +
                max - tmp3,
                NULL);
        xv_set(sortbutton, PANEL_LABEL_WIDTH,
                (int)xv_get(sortbutton, PANEL_LABEL_WIDTH) +
                max - tmp4,
                NULL);
        xv_set(bl->changebutton, PANEL_LABEL_WIDTH,
                (int)xv_get(bl->changebutton, PANEL_LABEL_WIDTH) +
                max - tmp5,
                NULL);

	c->browselist = (caddr_t)bl;
	init_blist(c);
}

browser_free_state()
{
	BrowserState *ptr, *s_ptr;

	ptr = state_list;

	while (ptr) {
		if (ptr->cname)
			free(ptr->cname);
		s_ptr = ptr;
		ptr = ptr->next;
		free(s_ptr);
	}
	state_list = NULL;
}
