#ifndef lint
static  char sccsid[] = "@(#)postup.c 3.9 97/05/13 Copyr 1991 Sun Microsystems, Inc.";
#endif

/* props.c */

#include <stdio.h>
#include <ctype.h>
#include "appt.h"
#include <sys/param.h>
#include <rpc/rpc.h>
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/textsw.h>
#include <xview/panel.h>
#include <xview/scrollbar.h>
#include	<xview/font.h>
#include <xview/svrimage.h>
#include "util.h"
#include "alarm.h"
#include "postup.h"
#include "graphics.h"
#include "calendar.h"
#include "timeops.h"
#include "gettext.h"
#include "datefield.h"
#include "props.h"


static  u_short postup_data[] = {
#include "postup.icon"
};

static Notify_value
postup_done(frame)
	Frame frame;
{
	xv_set(frame, XV_SHOW, FALSE, NULL);
	xv_destroy_safe(frame);
	return NOTIFY_DONE;
}

extern int debug;

extern caddr_t
make_postup()
{
	Postup *r = (Postup *) ckalloc(sizeof(Postup));

	/* There is going to be a bit of pretty lame trickery 
	   here.  The window manager enforces OPENLOOK.  This 
	   means that if the postup frame is a child of the 
	   calendar manager base frame, then it can never be
	   visible if the calendar manager base frame is closed 
	   (iconic).  To get around this, we create a second base 
	   frame that is invisible, and is always open.  We parent 
	   the command frame off of that, and make it visible.  
	   This produces a command frame whose stat of visibility 
	   is completely independent of the visible calendar manager 
	   base frame. */
	/* Also, we only make one of these and then hang the postups
	   off of it. There may be more than one postup at a time, so 
           we make and destroy the command frames as they are needed */ 
	

	r->bogus_frame = xv_create(XV_NULL, 		FRAME_BASE,
				FRAME_WM_COMMAND_ARGC_ARGV, NULL, -1,
				FRAME_CLOSED, 		FALSE,
				FRAME_NO_CONFIRM, 	TRUE,
				WIN_USE_IM, FALSE,
				XV_WIDTH, 1,
				XV_HEIGHT, 1,
				XV_SHOW, FALSE, 
				0);
	return (caddr_t)r;
}
static void
make_postup_frame(c, a)
	Calendar *c;
	Appt *a;
{
	struct pr_size size;
	Font_string_dims dims;
	Xv_Font pf;
	Rect *rect;
	Server_image	s1;
	Postup  *r = (Postup*)c->postup;
	Frame frame;
	Panel panel;
	Panel_item  date_string, time_string;
	int centerline, left_edge;
	long    time = a->appt_id.tick;
	char text[80];
	char            *tmp_copy, *t_p;
	struct tm       start_time, end_time;
        int             beg_hour, beg_min;

	centerline = 80;
	left_edge = 225;
	text[0] = '\0';

	frame = xv_create(r->bogus_frame, FRAME_CMD, 
		XV_HEIGHT, 180,
		XV_WIDTH, 520,
		FRAME_CLOSED, FALSE,
		FRAME_SHOW_LABEL, TRUE,
		FRAME_DONE_PROC, postup_done,
		XV_LABEL,  MGET("Calendar: Reminder") ,
		WIN_CLIENT_DATA, c,
		0);
	panel = xv_create(frame, PANEL, NULL);
	xv_set (panel,
		XV_X, 0,
		XV_Y, 0,
		WIN_CLIENT_DATA, c,
		WIN_ROW_GAP, 9,
		XV_HELP_DATA, "cm:ReminderPopup",
		0);

        format_tick(time, ((Props *)c->properties)->ordering_VAL,
                          ((Props *)c->properties)->separator_VAL,
                    text);
	start_time = *localtime(&time);

       	date_string = xv_create(panel, PANEL_MESSAGE,
                PANEL_LABEL_STRING, text,
                PANEL_LABEL_BOLD, TRUE,
                XV_Y, 10,
		XV_HELP_DATA, "cm:ReminderPopup",
                0);
	pf = (Xv_Font ) xv_get(date_string, PANEL_LABEL_FONT);

	(void) xv_get(pf, FONT_STRING_DIMS, text, &dims);
	size.x = dims.width;
	size.y = dims.height;
	xv_set(date_string,
                XV_X, centerline  - size.x/2,
		0);

	beg_hour = start_time.tm_hour;
	if (((Props *)c->properties)->default_disp_VAL == hour12) 
        	if (beg_hour > 12)
                	beg_hour -= 12;
		else if (beg_hour == 0)
                	beg_hour = 12;
        beg_min = start_time.tm_min;
        time = time + a->duration;
        end_time = *localtime(&time);
	if (((Props *)c->properties)->default_disp_VAL == hour12) 
        	if (end_time.tm_hour > 12)
                	end_time.tm_hour -= 12;

        if (a->tag->showtime && !magic_time(a->appt_id.tick)) 
		if (((Props *)c->properties)->default_disp_VAL == hour12)
                	sprintf(text, 
				MGET("From %2d:%2.2d to %2d:%2.2d"),
					 beg_hour, beg_min,
					 end_time.tm_hour, 
					end_time.tm_min);
                	else sprintf(text, 
				MGET("From %2d%2.2d to %2d%2.2d"),
					 beg_hour, beg_min,
					 end_time.tm_hour, 
					end_time.tm_min);
        else 
                text[0] = '\0';

	(void) xv_get(pf, FONT_STRING_DIMS, text, &dims);
	size.x = dims.width;
	size.y = dims.height;

       	time_string = xv_create(panel, PANEL_MESSAGE,
                PANEL_LABEL_STRING, text,
                PANEL_LABEL_BOLD, TRUE,
                XV_Y, 40,
                XV_X, centerline - size.x/2,
		XV_HELP_DATA, "cm:ReminderPopup",
		0);

	pf = (Xv_Font)xv_get(time_string, PANEL_LABEL_FONT);

	s1 = xv_create(XV_NULL, SERVER_IMAGE,
			SERVER_IMAGE_DEPTH, 1,
			XV_WIDTH, 64,
			XV_HEIGHT, 128,
			SERVER_IMAGE_BITS, postup_data,
			NULL);

       	xv_create(panel, PANEL_MESSAGE,
                PANEL_LABEL_IMAGE, s1,
                XV_Y, 60,
                XV_X, centerline  - 32,
		XV_HELP_DATA, "cm:ReminderPopup",
		0);

	tmp_copy = (char *) cm_strdup(a->what);
        t_p = (char *) strtok(tmp_copy, "\n");
        if (!t_p)
                t_p = "";
	sprintf(text, "%-45.45s", t_p);

	(void) xv_get(pf, FONT_STRING_DIMS, text, &dims);
	size.x = dims.width;
	size.y = dims.height;

       	xv_create(panel, PANEL_MESSAGE,
                PANEL_LABEL_STRING, text,
                PANEL_LABEL_BOLD, TRUE,
                XV_Y, 10,
                XV_X, left_edge,
		XV_HELP_DATA, "cm:ReminderPopup",
		0);

	t_p = (char *) strtok(NULL, "\n");
        if (!t_p)
                t_p = "";
        sprintf(text, "%-45.45s", t_p);

	(void) xv_get(pf, FONT_STRING_DIMS, text, &dims);
	size.x = dims.width;
	size.y = dims.height;

       	xv_create(panel, PANEL_MESSAGE,
                PANEL_LABEL_STRING, text,
                PANEL_LABEL_BOLD, TRUE,
                XV_Y, 45,
                XV_X, left_edge,
		XV_HELP_DATA, "cm:ReminderPopup",
		0);

	t_p = (char *) strtok(NULL, "\n");
        if (!t_p)
                t_p = "";
        sprintf(text, "%-45.45s", t_p);
	(void) xv_get(pf, FONT_STRING_DIMS, text, &dims);
	size.x = dims.width;
	size.y = dims.height;

       	xv_create(panel, PANEL_MESSAGE,
                PANEL_LABEL_STRING, text,
                PANEL_LABEL_BOLD, TRUE,
                XV_Y, 80,
                XV_X, left_edge,
		XV_HELP_DATA, "cm:ReminderPopup",
		0);

	t_p = (char *) strtok(NULL, "\n");
        if (!t_p)
                t_p = "";
        sprintf(text, "%-45.45s", t_p);
	(void) xv_get(pf, FONT_STRING_DIMS, text, &dims);
	size.x = dims.width;
	size.y = dims.height;

       	xv_create(panel, PANEL_MESSAGE,
                PANEL_LABEL_STRING, text,
                PANEL_LABEL_BOLD, TRUE,
                XV_Y, 115,
                XV_X, left_edge,
		XV_HELP_DATA, "cm:ReminderPopup",
		0);

	
	t_p = (char *) strtok(NULL, "\n");
        if (!t_p)
                t_p = "";
        sprintf(text, "%-45.45s", t_p);

	(void) xv_get(pf, FONT_STRING_DIMS, text, &dims);
	size.x = dims.width;
	size.y = dims.height;

       	xv_create(panel, PANEL_MESSAGE,
                PANEL_LABEL_STRING, text,
                PANEL_LABEL_BOLD, TRUE,
                XV_Y, 150,
                XV_X, left_edge,
		XV_HELP_DATA, "cm:ReminderPopup",
		0);

	free(tmp_copy);

	window_fit(frame);

	rect = (Rect *) xv_get(frame, WIN_RECT);

	xv_set(frame, 
		WIN_X, 550 - rect->r_width/2,
		WIN_Y, 450 - rect->r_height/2,
		0);

	xv_set(frame, FRAME_CMD_PUSHPIN_IN, TRUE, NULL);

	xv_set(frame, XV_SHOW, TRUE, NULL);
}
extern void
postup_show_proc(frame, a)
	Frame	frame;
	Appt 	*a;		/* laf */
{
	Calendar *c;
	Postup *r;

	c = (Calendar *) xv_get(frame, WIN_CLIENT_DATA, 0);
	if(c==NULL) return;
	r = (Postup *) c->postup;

	if (r==NULL) 
		make_postup();

	make_postup_frame(c, a);
}
