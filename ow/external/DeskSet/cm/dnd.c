#ifndef lint
static  char sccsid[] = "@(#)dnd.c 1.14 93/12/22 Copyr 1991 Sun Microsystems, Inc.";
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

#include <xview/frame.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/dragdrop.h>
#include <xview/cursor.h>
#include <xview/notice.h>
#ifndef SVR4
#include <xview/font.h>
#endif
#include <gdd.h>
#include "gettext.h"
#include "util.h"
#include "calendar.h"
#include "appt.h"
#include "editor.h"
#include "common.h"

extern int debug;

static void
get_drop_info_data(data, size, apptptr)
	char *data;
	int size;
	Appt **apptptr;
{
	char filename[15];
	FILE *fd;

	strcpy(filename, "/tmp/cmXXXXXX");
	mktemp(filename);

	fd = fopen(filename, "w");
	if (fd) {
		fwrite(data, 1, size, fd);
		fclose(fd);

		drag_load_proc(filename, apptptr);
		unlink(filename);
	} else
		notice_prompt(calendar->frame, (Event *)NULL,
			NOTICE_MESSAGE_STRINGS,
			EGET("Unable to display data."),
			EGET("Failed to get the transferred data."),
			NULL,
			NOTICE_BUTTON_YES, LGET("Continue"),
			NULL);
}

void
drop_on_canvas(Xv_opaque item, Event *event, GDD_DROP_INFO *drop_info)
{
	char filename[15];

	if (debug)
		gdd_print_drop_info(drop_info);

	if ((drop_info->source_host &&
	     strcmp(drop_info->source_host, cm_get_local_host()) == 0) &&
	    drop_info->filename) {

		drag_load_proc(drop_info->filename, NULL);

	} else if (drop_info->data) {

		get_drop_info_data(drop_info->data, drop_info->length, NULL);
	}
}

void
drop_on(Xv_opaque item, Event *event, GDD_DROP_INFO *drop_info)
{
	char filename[15];
	Appt *appt = NULL;

	if (debug)
		gdd_print_drop_info(drop_info);

	if ((drop_info->source_host &&
	     strcmp(drop_info->source_host, cm_get_local_host()) == 0) &&
	    drop_info->filename) {

		drag_load_proc(drop_info->filename, &appt);

	} else if (drop_info->data) {

		get_drop_info_data(drop_info->data, drop_info->length, &appt);
	}

	if (appt) {
		show_appt(appt, false);
		destroy_appt(appt);
	}
}

/*
 * Drag callback function for `droptarget'.
 */
void
drag_from(Xv_opaque item,
	Event *event,
	GDD_DROP_INFO *drop_info,
	int drag_state)
{
	char *appt;

	switch (drag_state) {
	case GDD_DRAG_STARTED:
		appt = get_appt_str();
		drop_info->app_name = strdup("cm");
		drop_info->data_label = NULL;
		drop_info->data = appt;
		drop_info->length = appt ? strlen(appt) : 0;

		xv_set(item, PANEL_CLIENT_DATA, appt, NULL);
		break;

	case GDD_DRAG_COMPLETED:
		appt = (char *)xv_get(item, PANEL_CLIENT_DATA);
		if (appt) 
			free(appt);
		break;

	}
}

make_dnd(e)
	Editor *e;
{
	Drag_drop       dnd;
        Selection_item  sel_item;
        Cursor          dnd_cursor;
        Cursor          dnd_accept_cursor;
	Xv_opaque               droptarget_accept_cursor_image;
        static unsigned short   droptarget_accept_cursor_bits[] = {
#include "accept.cursor"
        };
        Xv_opaque               droptarget_cursor_image;
        static unsigned short   droptarget_cursor_bits[] = {
#include "drag.cursor"
        };

	droptarget_accept_cursor_image = xv_create(XV_NULL, SERVER_IMAGE,
                SERVER_IMAGE_DEPTH, 1,
                SERVER_IMAGE_BITS, droptarget_accept_cursor_bits,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                NULL);  
        droptarget_cursor_image = xv_create(XV_NULL, SERVER_IMAGE,
                SERVER_IMAGE_DEPTH, 1,
                SERVER_IMAGE_BITS, droptarget_cursor_bits,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                NULL);
         
        dnd_cursor = xv_create(XV_NULL, CURSOR,
                CURSOR_IMAGE, droptarget_cursor_image,
                CURSOR_OP, PIX_SRC^PIX_DST,
                CURSOR_XHOT, 15,
                CURSOR_YHOT, 20,
                NULL);
                
        dnd_accept_cursor = xv_create(XV_NULL, CURSOR,
                CURSOR_IMAGE, droptarget_accept_cursor_image,
                CURSOR_OP, PIX_SRC^PIX_DST,
                CURSOR_XHOT, 15,
                CURSOR_YHOT, 20,
                NULL);
                
        e->dnd_target = xv_create(e->panel, PANEL_DROP_TARGET,
                XV_X, xv_get(e->appt_type, XV_X) + 
			xv_get(e->appt_type, XV_WIDTH) + 20,
                XV_Y, xv_row(e->panel, 0) + 10,
                XV_HELP_DATA, "cm:DnDTarget",
                NULL);

        dnd = xv_create(e->panel, DRAGDROP,
		DND_CURSOR, dnd_cursor,
                DND_ACCEPT_CURSOR, dnd_accept_cursor,
                NULL);
                
        sel_item = xv_create(dnd, SELECTION_ITEM, NULL);
         
        xv_set(e->dnd_target,
                PANEL_NOTIFY_PROC, gdd_drop_target_notify_proc,
                PANEL_DROP_DND, dnd,
		PANEL_DROP_FULL, TRUE,
                XV_KEY_DATA, SELECTION_ITEM, sel_item,
                NULL);

	gdd_register_drop_target(e->dnd_target, drop_on, drag_from);
}

init_dragdrop()
{
	Rect	absolute_rect[2];
	Xv_drop_site	drop_site;

	/* make the main canvas a drop site */
    	drop_site = xv_create(calendar->canvas, DROP_SITE_ITEM,
		DROP_SITE_DEFAULT,	TRUE,
                DROP_SITE_ID,           1,
                DROP_SITE_EVENT_MASK, DND_MOTION | DND_ENTERLEAVE,
                0);

	notify_interpose_event_func(calendar->canvas,
		(Notify_func)gdd_load_event_proc, NOTIFY_SAFE);

	absolute_rect[0] = *(Rect *)xv_get(calendar->canvas, XV_RECT);
	absolute_rect[1].r_top = 0;
	absolute_rect[1].r_left = 0;
	absolute_rect[1].r_width = 0;
	absolute_rect[1].r_height = 0;

	gdd_register_drop_site(drop_site, drop_on_canvas);
	gdd_activate_drop_site(drop_site, absolute_rect);

	gdd_init_dragdrop(calendar->panel);
}

