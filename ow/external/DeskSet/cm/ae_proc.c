#ifndef lint
static  char sccsid[] = "@(#)ae_proc.c 1.25 93/12/22 Copyr 1991 Sun Microsystems, Inc.";
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
#include <string.h>
#include <netdb.h>
#ifdef SVR4
#include <sys/systeminfo.h>
#else
#include <sys/param.h>
#endif
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/notice.h>
#include <time.h>
#include <gdd.h>
#include "ds_popup.h"
#include "gettext.h"
#include "misc.h"
#include "ae_ui.h"
#include "ae.h"
#include "ae_tt.h"
#include "repeat.h"

extern char *cm_strdup(char *);

#define NOTICE_CANCEL 2

extern char *lastappt;

static char *lhost = NULL;
static int datefilled, startfilled, whatfilled, dnd_full;

typedef enum {dstoff, dston, nochange} DSTchange;

static void
set_dnd_target()
{
	if ((datefilled && startfilled) || (datefilled && whatfilled)) {
		if (dnd_full == false) {
			xv_set(Ae_window->droptarget,
				PANEL_DROP_FULL, TRUE, NULL);
			dnd_full = true;
		}
	} else if (dnd_full) {
		xv_set(Ae_window->droptarget, PANEL_DROP_FULL, FALSE, NULL);
		dnd_full = false;
	}
}

/*
 * Get appt info from ui objects.
 */
extern char *
get_appt()
{
	ae_window_objects *ip = Ae_window;
	char	*value;
	int	periodval, nthval, wk, alert_res;
	Rc_value *rc;
	char	*whatbuf;
	char	buf[BUFSIZ], date_buf[100], start_buf[15], stop_buf[15];
	char	appt_buf[BUFSIZ], repeat_buf[40];
	long	start_tick;

	start_buf[0] = '\0';
	stop_buf[0] = '\0';

	rc = (Rc_value *)xv_get(ip->window, WIN_CLIENT_DATA);

	/* get and validate date */
	value = (char *)xv_get(ip->date_textfield, PANEL_VALUE);
	if (value == NULL || *value == '\0') {
		notice_prompt(ip->window, (Event *)NULL,
				NOTICE_MESSAGE_STRINGS,
				EGET("Please enter a date in the Date Field"),
				NULL,
				NOTICE_BUTTON_YES, LGET("Continue"),
				NULL);
		return NULL;
	}

	if ((get_datestr(date_buf, value, rc)) == -1) {
		notice_prompt(ip->window, (Event *)NULL,
				NOTICE_MESSAGE_STRINGS,
				EGET("Error in Date Field."),
				NULL,
				NOTICE_BUTTON_YES, LGET("Continue"),
				NULL);
		return NULL;
	}

	/* get and validate start and stop time */
	value = (char *)xv_get(ip->start_textfield, PANEL_VALUE);
	if (value != NULL && *value != '\0') {
		/* check validity of start time */
		cm_strcpy(start_buf, value);
		if (!rc || rc->hour24 == 0)
			cm_strcat(start_buf, (xv_get(ip->start_ampm,
					PANEL_VALUE) ? "pm" : "am"));
		sprintf(buf, "%s %s", date_buf, start_buf);
		if ((start_tick = cm_getdate(buf, NULL)) == -1) {
			notice_prompt(ip->window, (Event *)NULL,
				NOTICE_MESSAGE_STRINGS,
				EGET("Error in Start Field."),
				NULL,
				NOTICE_BUTTON_YES, LGET("Continue"),
				NULL);
			return NULL;
		}

		value = (char *)xv_get(ip->stop_textfield, PANEL_VALUE);
		if (value != NULL && *value != '\0') {
			cm_strcpy(stop_buf, value);
			if (!rc || rc->hour24 == 0)
				cm_strcat(stop_buf,
					(xv_get(ip->stop_ampm,PANEL_VALUE) ?
					"pm" : "am"));
			sprintf(buf, "%s %s", date_buf, stop_buf);
			if (cm_getdate(buf, NULL) == -1) {
				notice_prompt(ip->window, (Event *)NULL,
					NOTICE_MESSAGE_STRINGS,
					EGET("Error in Stop Field."),
					NULL,
					NOTICE_BUTTON_YES, LGET("Continue"),
					NULL);
				return NULL;
			}
		}
	} else {
		value = (char *)xv_get(ip->stop_textfield, PANEL_VALUE);
		if (value != NULL && *value != '\0') {
			notice_prompt(ip->window, (Event *)NULL,
				NOTICE_MESSAGE_STRINGS,
				EGET("Specify a START time..."),
				NULL,
				NOTICE_BUTTON_YES, LGET("Continue"),
				NULL);
			return NULL;
		}
	}

	/* what */
	buf[0] = '\0';
	whatbuf = (char *)xv_get(ip->what_textfield1, PANEL_VALUE);
	if (!blank_buf(whatbuf)) {
		cm_strcat(buf, whatbuf);
		cm_strcat(buf, "\n\t");
	}
	whatbuf = (char *)xv_get(ip->what_textfield2, PANEL_VALUE);
	if (!blank_buf(whatbuf)) {
		cm_strcat(buf, whatbuf);
		cm_strcat(buf, "\n\t");
	}
	whatbuf = (char *)xv_get(ip->what_textfield3, PANEL_VALUE);
	if (!blank_buf(whatbuf)) {
		cm_strcat(buf, whatbuf);
		cm_strcat(buf, "\n\t");
	}
	whatbuf = (char *)xv_get(ip->what_textfield4, PANEL_VALUE);
	if (!blank_buf(whatbuf)) {
		cm_strcat(buf, whatbuf);
		cm_strcat(buf, "\n");
	}

	if (start_buf[0] == NULL && stop_buf[0] == NULL && blank_buf(buf)) {
		notice_prompt(ip->window, (Event *)NULL,
		    NOTICE_MESSAGE_STRINGS,
		    EGET("Bad Appointment: No START, END, and WHAT specified."),
		    NULL,
		    NOTICE_BUTTON_YES, LGET("Continue"),
		    NULL);
		return NULL;
	}

	periodval = (int)xv_get(ip->repeat_message, PANEL_CLIENT_DATA);
	nthval = (int)xv_get(ip->repeat_button, PANEL_CLIENT_DATA);
	if (periodval == nthWeekday && nthval == 0) {
		if (weekofmonth(start_tick, &wk) && wk == 4) {
			/* ambiguous case */
			alert_res = notice_prompt(ip->window, (Event *)NULL,
				NOTICE_MESSAGE_STRINGS,
				MGET("Is this appointment in the Last Week of the Month or"),
				MGET("the 4th Week of the Month?"),
				NULL,
				NOTICE_BUTTON_YES, LGET("Last Week"),
				NOTICE_BUTTON_NO, LGET("4th Week"),
				NOTICE_BUTTON, LGET("Cancel"), NOTICE_CANCEL,
				NULL);
			if (alert_res == NOTICE_CANCEL)
				return NULL;
			else if (alert_res == NOTICE_YES)
				nthval = -1;
			else
				nthval = wk;
		} else if (wk == 5)
			nthval = -1;
		else
			nthval = wk;
	}

	cm_strcpy(repeat_buf, period_to_Clocstr(periodval, nthval)); 
	if (periodval == nthWeekday) {
		if (nthval == -1)
			cm_strcat(repeat_buf, ", last");
		else if (nthval == 4)
			cm_strcat(repeat_buf, ", 4th");
	}

	sprintf(appt_buf, "\n\n\t** Calendar Appointment **\n\n\tDate: %s\n\tStart: %s\n\tEnd: %s\n\tRepeat: %s\n%s%s%s\tWhat: %s\n",
		date_buf, start_buf, stop_buf, repeat_buf,
		(periodval ? "\tFor: " : ""),
		(periodval ? (char *)xv_get(ip->for_textfield,PANEL_VALUE):""),
		(periodval ? "\n": ""), buf);

	return(cm_strdup(appt_buf));
}

extern void
check_dnd_fullness(Miniappt *appt)
{
	/* determine if the date, start, and what fields are filled */
	if (appt->formattedstr && (cm_strlen(appt->formattedstr) > (size_t)0))
		datefilled = TRUE;
	else
		datefilled = FALSE;

	if (appt->showstart == TRUE)
		startfilled = TRUE;
	else
		startfilled = FALSE;

	whatfilled = FALSE;
	if (appt->what1 && (cm_strlen(appt->what1) > (size_t)0))
		whatfilled = TRUE;
	else if (appt->what2 && (cm_strlen(appt->what2) > (size_t)0))
		whatfilled = TRUE;
	else if (appt->what3 && (cm_strlen(appt->what3) > (size_t)0))
		whatfilled = TRUE;
	else if (appt->what4 && (cm_strlen(appt->what4) > (size_t)0))
		whatfilled = TRUE;

	set_dnd_target();
}

extern void
set_default_values(ae_window_objects *ip)
{
	time_t		today;
	struct tm	*tm;
	char		buff[100];
	Ordering_Type	order;
	Rc_value	*rc = NULL;

	rc = (Rc_value *)xv_get(ip->window, WIN_CLIENT_DATA);

	/* set date field */
	today = time(0);
	format_tick(today, (rc ? rc->order : Order_DMY),
		(rc ? rc->sep : Separator_Slash), buff);
	xv_set(ip->date_textfield, PANEL_VALUE, buff, NULL);

	if (rc == NULL || rc->hour24 == 0) {
		xv_set(ip->start_textfield, PANEL_VALUE, "9:00", NULL);
		xv_set(ip->start_ampm, PANEL_VALUE, 0, NULL);
		xv_set(ip->stop_textfield, PANEL_VALUE, "10:00", NULL);
		xv_set(ip->stop_ampm, PANEL_VALUE, 0, NULL);
	} else {
		xv_set(ip->start_textfield, PANEL_VALUE, "0900",
			PANEL_VALUE_DISPLAY_LENGTH, 17, NULL);
		xv_set(ip->start_ampm, PANEL_VALUE, 0, XV_SHOW, FALSE, NULL);
		xv_set(ip->stop_textfield, PANEL_VALUE, "1000",
			PANEL_VALUE_DISPLAY_LENGTH, 17, NULL);
		xv_set(ip->stop_ampm, PANEL_VALUE, 0, XV_SHOW, FALSE, NULL);
	}

	xv_set(ip->repeat_message, PANEL_LABEL_STRING, periodstr[onetime],
		PANEL_CLIENT_DATA, 0, NULL);
	xv_set(ip->for_button, PANEL_INACTIVE, TRUE, NULL);
	xv_set(ip->for_textfield, PANEL_INACTIVE, TRUE,
		PANEL_VALUE, repeatval[onetime], NULL);
	xv_set(ip->days_message, PANEL_INACTIVE, TRUE,
		PANEL_LABEL_STRING, repeatstr[onetime], NULL);
	xv_set(ip->what_textfield1, PANEL_VALUE, "", NULL);
	xv_set(ip->what_textfield2, PANEL_VALUE, "", NULL);
	xv_set(ip->what_textfield3, PANEL_VALUE, "", NULL);
	xv_set(ip->what_textfield4, PANEL_VALUE, "", NULL);
	xv_set(ip->controls,
		PANEL_CARET_ITEM, Ae_window->what_textfield1, NULL);

	datefilled = startfilled = TRUE;
	whatfilled = FALSE;
	set_dnd_target();

	xv_set(ip->window, FRAME_LEFT_FOOTER, "", NULL);
}

static int
get_datestr(char *buf, char *datestr, Rc_value *rc)
{
	datestr2mdy(datestr, (rc ? rc->order : Order_MDY),
		(rc ? rc->sep : Separator_Slash), buf);

	if (cm_getdate(buf, NULL) == -1)
		return -1;
	else
		return 0;
}

static void
stop_hr_proc(Menu m, Menu_item mi)
{
	ae_window_objects *ip =
		(ae_window_objects *) xv_get(m, XV_KEY_DATA, INSTANCE);
	char		*secstr;
	int		hr;
	char		*hrstr;
	char		buff[30];
	Rc_value	*rc;

	hr = (int)xv_get(mi, MENU_CLIENT_DATA);
	hrstr = (char *)xv_get(mi, MENU_STRING);

	if ((secstr = (char *)xv_get(m, MENU_CLIENT_DATA)) == NULL)
		secstr = "00";
	else
		xv_set(m, MENU_CLIENT_DATA, NULL, NULL);

	rc = (Rc_value *)xv_get(ip->window, WIN_CLIENT_DATA);

	if (!rc || rc->hour24 == 0) {
		if (hr > 11)
			xv_set(ip->stop_ampm, PANEL_VALUE, 1, NULL);
		else
			xv_set(ip->stop_ampm, PANEL_VALUE, 0, NULL);
	}

	if (hr == -1)
		xv_set(ip->stop_textfield, PANEL_VALUE, "", NULL);
	else {
		sprintf(buff, "%s%s", hrstr, secstr);
		xv_set(ip->stop_textfield, PANEL_VALUE, buff, NULL);
	}
	data_changed = TRUE;
	xv_set(m, MENU_NOTIFY_STATUS, XV_ERROR, NULL);
}

static void
start_hr_proc(Menu m, Menu_item mi)
{
	ae_window_objects *ip =
		(ae_window_objects *) xv_get(m, XV_KEY_DATA, INSTANCE);
	char		*secstr;
	int		hr;
	char		*hrstr;
	char		buff[30];
	Rc_value	*rc;
	time_t		t, t1;
	struct tm	*tm;

	hr = (int)xv_get(mi, MENU_CLIENT_DATA);
	hrstr = (char *)xv_get(mi, MENU_STRING);

	if ((secstr = (char *)xv_get(m, MENU_CLIENT_DATA)) == NULL)
		secstr = "00";
	else
		xv_set(m, MENU_CLIENT_DATA, NULL, NULL);

	rc = (Rc_value *)xv_get(ip->window, WIN_CLIENT_DATA);
	
	if (!rc || rc->hour24 == 0) {
		if (hr > 11)
			xv_set(ip->start_ampm, PANEL_VALUE, 1, NULL);
		else
			xv_set(ip->start_ampm, PANEL_VALUE, 0, NULL);
	}

	if (hr == -1)
		startfilled = FALSE;
	else
		startfilled = TRUE;
	set_dnd_target();

	if (hr == -1) {
		xv_set(ip->start_textfield, PANEL_VALUE, "", NULL);
		xv_set(ip->stop_textfield, PANEL_VALUE, "", NULL);
		xv_set(ip->stop_ampm, PANEL_VALUE, 0, NULL);
	} else if (hr == -2) {
		if (!rc || rc->hour24 == 0) {
			xv_set(ip->start_textfield, PANEL_VALUE, "12:00", NULL);
			xv_set(ip->stop_textfield, PANEL_VALUE, "11:59", NULL);
			xv_set(ip->stop_ampm, PANEL_VALUE, 1, NULL);
		} else {
			xv_set(ip->start_textfield,
				PANEL_VALUE, "0000", NULL);
				xv_set(ip->stop_textfield,
				PANEL_VALUE, "2359", NULL);
		}
	} else {
		sprintf(buff, "%s%s", hrstr, secstr);
		xv_set(ip->start_textfield, PANEL_VALUE, buff, NULL);
	}

	if (hr != -1 && hr != -2) {
		if (!rc || rc->hour24 == 0) {
			if ((hr+1) > 11)
				xv_set(ip->stop_ampm, PANEL_VALUE, 1, NULL);
			else
				xv_set(ip->stop_ampm, PANEL_VALUE, 0, NULL);
			if (hr == 23 &&
			    (xv_get(ip->start_ampm, PANEL_VALUE) == 1))
				xv_set(ip->stop_ampm, PANEL_VALUE, 0, NULL);
		}
		get_datestr(buff, (char *)xv_get(ip->date_textfield,
				PANEL_VALUE), rc);
		cm_strcat(buff, " ");
		cm_strcat(buff, (char *)xv_get(ip->start_textfield, PANEL_VALUE));
		if (!rc || rc->hour24 == 0)
			cm_strcat(buff, xv_get(ip->start_ampm, PANEL_VALUE) ?
				"pm" : "am");
		t = (int)cm_getdate(buff, NULL);
		t1 = t + HOURSEC;
		t1 = adjust_dst(t, t1);
		tm = localtime(&t1);
		hr = tm->tm_hour;
		if (!rc || rc->hour24 == 0) {
			adjust_hour(&hr);
			sprintf(buff, "%2d:%02d", hr, tm->tm_min);
		} else
			sprintf(buff, "%02d%02d", hr, tm->tm_min);
		xv_set(ip->stop_textfield, PANEL_VALUE, buff, NULL);
	}
	data_changed = TRUE;
	xv_set(m, MENU_NOTIFY_STATUS, XV_ERROR, NULL);
}

static void
min_proc(Menu m, Menu_item mi, void (func)())
{
	char *sec;
        Menu_item time_item;
	Menu parent_menu=NULL;

	data_changed = TRUE;
	sec = (char *) xv_get(mi, MENU_STRING, NULL);
	time_item = (Menu_item)xv_get(m, MENU_PARENT, NULL);
	parent_menu = xv_get(time_item, MENU_PARENT);
	xv_set(parent_menu, MENU_CLIENT_DATA, sec, NULL);
	func(parent_menu, time_item);
}

static void
start_min_proc(m, mi)
	Menu m;
	Menu_item mi;
{
	min_proc(m, mi, start_hr_proc);
}

static void
stop_min_proc(m, mi)
	Menu m;
	Menu_item mi;
{
	min_proc(m, mi, stop_hr_proc);
}

static Xv_opaque
ae_make_start_menu(ae_window_objects *ip)
{
	int i=0, j=0;
	char buf[10];
	Menu minutemenu;
        Menu timemenu;
	Menu_item item;
	Rc_value *rc = NULL;
	
	if (ip==NULL) return(NULL);

	rc = (Rc_value *)xv_get(ip->window, WIN_CLIENT_DATA);

	timemenu = (Menu)xv_get(ip->start_button, PANEL_ITEM_MENU);
	if (timemenu != NULL)
		menu_destroy(timemenu);

	minutemenu = menu_create(
		XV_KEY_DATA, INSTANCE, ip,
		MENU_ITEM, 
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, "00",
			MENU_CLIENT_DATA, 0,
			MENU_NOTIFY_PROC, start_min_proc,
			NULL,
		MENU_ITEM, 
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, "15",
			MENU_CLIENT_DATA, 15,
			MENU_NOTIFY_PROC, start_min_proc,
			NULL,
		MENU_ITEM, 
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, "30",
			MENU_CLIENT_DATA, 30,
			MENU_NOTIFY_PROC, start_min_proc,
			NULL,
		MENU_ITEM, 
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, "45",
			MENU_CLIENT_DATA, 45,
			MENU_NOTIFY_PROC, start_min_proc,
			NULL,
		XV_HELP_DATA, "cm:MaeApptTime",
		NULL);

	timemenu = menu_create(
		XV_KEY_DATA, INSTANCE, ip,
		MENU_NOTIFY_PROC, start_hr_proc,
		MENU_NOTIFY_STATUS, XV_ERROR,
		XV_HELP_DATA, "cm:MaeApptTime",
		NULL);

	i = (rc ? rc->daybegin : DEFDAYBEGIN);
	j = (rc ? rc->dayend : DEFDAYEND);

	for(; i < j; i++) {
		if (rc == NULL || rc->hour24 == 0) {
			if (i > 12)
                       		 sprintf(buf, "%2d:", i-12);
                	else
                        	if (i == 0)
                                	cm_strcpy(buf, "12:");
                	else
                       		 sprintf(buf, "%2d:", i);	
		} else
			sprintf(buf, "%02d", i);

		item = menu_create_item(MENU_PULLRIGHT_ITEM,
				cm_strdup(buf), minutemenu,
				MENU_NOTIFY_PROC, start_hr_proc,
				XV_KEY_DATA, INSTANCE, ip,
				MENU_CLIENT_DATA, i,
				NULL);
		menu_set(timemenu, MENU_APPEND_ITEM, item, NULL);
	}

	item = menu_create_item( MENU_STRING,  LGET("No Time"),
		MENU_CLIENT_DATA,  -1,
		MENU_NOTIFY_PROC, start_hr_proc,
		XV_KEY_DATA, INSTANCE, ip,
		NULL);
	menu_set(timemenu,
		MENU_APPEND_ITEM, item,
		NULL);

	item = menu_create_item( MENU_STRING,  LGET("All Day") ,
		MENU_CLIENT_DATA,  -2,
		MENU_NOTIFY_PROC, start_hr_proc,
		XV_KEY_DATA, INSTANCE, ip,
		NULL);
        menu_set(timemenu,
		MENU_APPEND_ITEM, item,
		NULL);

	return(timemenu);
}

static Xv_opaque
ae_make_stop_menu(ae_window_objects *ip)
{
	int i=0, j=0;
	char buf[10];
	Menu minutemenu;
        Menu timemenu;
	Menu_item item;
	Rc_value *rc = NULL;
	
	if (ip==NULL) return(NULL);

	rc = (Rc_value *)xv_get(ip->window, WIN_CLIENT_DATA);

	timemenu = (Menu)xv_get(ip->stop_button, PANEL_ITEM_MENU);
	if (timemenu != NULL)
		menu_destroy(timemenu);

	minutemenu = menu_create(
		XV_KEY_DATA, INSTANCE, ip,
		MENU_ITEM, 
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, "00",
			MENU_CLIENT_DATA, 0,
			MENU_NOTIFY_PROC, stop_min_proc,
			NULL,
		MENU_ITEM, 
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, "15",
			MENU_CLIENT_DATA, 15,
			MENU_NOTIFY_PROC, stop_min_proc,
			NULL,
		MENU_ITEM, 
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, "30",
			MENU_CLIENT_DATA, 30,
			MENU_NOTIFY_PROC, stop_min_proc,
			NULL,
		MENU_ITEM, 
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, "45",
			MENU_CLIENT_DATA, 45,
			MENU_NOTIFY_PROC, stop_min_proc,
			NULL,
		XV_HELP_DATA, "cm:MaeApptDuration",
		NULL);

	timemenu = menu_create(
		XV_KEY_DATA, INSTANCE, ip,
		MENU_NOTIFY_PROC, stop_hr_proc,
		MENU_NOTIFY_STATUS, XV_ERROR,
		XV_HELP_DATA, "cm:MaeApptDuration",
		NULL);

	i = (rc ? rc->daybegin : DEFDAYBEGIN);
	j = (rc ? rc->dayend : DEFDAYEND);

	for(; i < j; i++) {
		if (rc == NULL || rc->hour24 == 0) {
			if (i > 12)
                       		 sprintf(buf, "%2d:", i-12);
                	else
                        	if (i == 0)
                                	cm_strcpy(buf, "12:");
                	else
                       		 sprintf(buf, "%2d:", i);	
		} else
			sprintf(buf, "%02d", i);

		item = menu_create_item(MENU_PULLRIGHT_ITEM,
				cm_strdup(buf), minutemenu,
				MENU_NOTIFY_PROC, stop_hr_proc,
				XV_KEY_DATA, INSTANCE, ip,
				MENU_CLIENT_DATA, i,
				NULL);
		menu_set(timemenu, MENU_APPEND_ITEM, item, NULL);
	}

	item = menu_create_item( MENU_STRING,  LGET("No Time"),
		MENU_CLIENT_DATA,  -1,
		MENU_NOTIFY_PROC, stop_hr_proc,
		XV_KEY_DATA, INSTANCE, ip,
		NULL);
	menu_set(timemenu, MENU_APPEND_ITEM, item, NULL);

	return(timemenu);
}

static void
set_repeat_proc(Menu m, Menu_item mi)
{
	ae_window_objects *ip = (ae_window_objects *)xv_get(mi, XV_KEY_DATA,
					INSTANCE);
	int value = (int)xv_get(mi, MENU_VALUE);

	xv_set(ip->repeat_message,
		PANEL_LABEL_STRING, (char *)xv_get(mi, MENU_STRING),
		PANEL_CLIENT_DATA, value,
		NULL);
	if (value) {
		xv_set(ip->repeat_button, PANEL_CLIENT_DATA, 0, NULL);
		xv_set(ip->for_button, PANEL_INACTIVE, FALSE, NULL);
		xv_set(ip->for_textfield, PANEL_INACTIVE, FALSE,
			PANEL_VALUE, repeatval[value],
			NULL);
		xv_set(ip->days_message, PANEL_INACTIVE, FALSE,
			PANEL_LABEL_STRING, repeatstr[value],
			NULL);
	} else {
		xv_set(ip->for_button, PANEL_INACTIVE, TRUE, NULL);
		xv_set(ip->for_textfield, PANEL_INACTIVE, TRUE,
			PANEL_VALUE, repeatval[value],
			NULL);
		xv_set(ip->days_message, PANEL_INACTIVE, TRUE,
			PANEL_LABEL_STRING, repeatstr[value],
			NULL);
	}
	data_changed = TRUE;
	xv_set(m, MENU_NOTIFY_STATUS, XV_ERROR, NULL);
}

static void
show_repeat(m , mi)
        Menu m;
        Menu_item mi;
{
	static int position = TRUE;
	if (position) {
		position = FALSE;
		ds_position_popup(Ae_window->window, Ae_repeat_win->repeat_win,
			DS_POPUP_LOR);
	}
	(void)xv_set(Ae_repeat_win->repeat_win, XV_SHOW, TRUE, NULL);
}

static Xv_opaque
ae_make_repeat_menu(ae_window_objects *ip)
{
        Menu repeatmenu;
	Menu item;
	int i;

	if (ip==NULL) return(NULL);

        repeatmenu = menu_create(
		XV_KEY_DATA, INSTANCE, ip,
                MENU_NOTIFY_STATUS, XV_ERROR,
                MENU_ITEM,
                        MENU_STRING,  periodstr[0] ,
                        MENU_VALUE, 0,
                	MENU_NOTIFY_PROC, set_repeat_proc,
			XV_KEY_DATA, INSTANCE, ip,
                        0,
                MENU_ITEM,
                        MENU_STRING,  periodstr[1],
                        MENU_VALUE, 1,
                	MENU_NOTIFY_PROC, set_repeat_proc,
			XV_KEY_DATA, INSTANCE, ip,
                        0,
                MENU_ITEM,
                        MENU_STRING,  periodstr[2],
                        MENU_VALUE, 2,
                	MENU_NOTIFY_PROC, set_repeat_proc,
			XV_KEY_DATA, INSTANCE, ip,
                        0,
                MENU_ITEM,
                        MENU_STRING,  periodstr[3],
                        MENU_VALUE, 3,
                	MENU_NOTIFY_PROC, set_repeat_proc,
			XV_KEY_DATA, INSTANCE, ip,
                        0,
                MENU_ITEM,
                        MENU_STRING,  periodstr[4],
                        MENU_VALUE, 4,
                	MENU_NOTIFY_PROC, set_repeat_proc,
			XV_KEY_DATA, INSTANCE, ip,
                        0,
                MENU_ITEM,
                        MENU_STRING,  periodstr[6],
                        MENU_VALUE, 6,
                	MENU_NOTIFY_PROC, set_repeat_proc,
			XV_KEY_DATA, INSTANCE, ip,
                        0,
                MENU_ITEM,
                        MENU_STRING,  periodstr[5],
                        MENU_VALUE, 5,
                	MENU_NOTIFY_PROC, set_repeat_proc,
			XV_KEY_DATA, INSTANCE, ip,
                        0,
                MENU_ITEM,
                        MENU_STRING,  periodstr[11],
                        MENU_VALUE, 11,
                	MENU_NOTIFY_PROC, set_repeat_proc,
			XV_KEY_DATA, INSTANCE, ip,
                        0,
                MENU_ITEM,
                        MENU_STRING,  periodstr[12],
                        MENU_VALUE, 12,
                	MENU_NOTIFY_PROC, set_repeat_proc,
			XV_KEY_DATA, INSTANCE, ip,
                        0,
                MENU_ITEM,
                        MENU_STRING,  periodstr[13],
                        MENU_VALUE, 13,
                	MENU_NOTIFY_PROC, set_repeat_proc,
			XV_KEY_DATA, INSTANCE, ip,
                        0,
                MENU_ITEM,
                        MENU_STRING,  LGET("Repeat Every...") ,
                        MENU_VALUE, 7,
			MENU_ACTION_PROC, show_repeat, 
			XV_KEY_DATA, INSTANCE, ip,
                        0,
                XV_HELP_DATA, "cm:RepeatUnit",
                NULL);

	return(repeatmenu);
}

static void
set_for_proc(Menu m, Menu_item mi)
{
	ae_window_objects *ip = (ae_window_objects *)xv_get(m, XV_KEY_DATA,
					INSTANCE);

	xv_set(ip->for_textfield, PANEL_VALUE, (char *)xv_get(mi, MENU_STRING),
		NULL);
	xv_set(m, MENU_NOTIFY_STATUS, XV_ERROR, NULL);
	data_changed = TRUE;
}

static Xv_opaque
ae_make_for_menu(ae_window_objects *ip)
{
        Menu formenu;
	Menu item;
	int i;
	char buf[3];

	if (ip==NULL) return(NULL);

	formenu = menu_create(
		XV_KEY_DATA, INSTANCE, ip,
		MENU_NOTIFY_PROC, set_for_proc,
		MENU_NOTIFY_STATUS, XV_ERROR,
		MENU_ITEM,
			MENU_STRING, "2",
			MENU_CLIENT_DATA, 2,
			NULL,
		MENU_ITEM,
			MENU_STRING, "3",
			MENU_CLIENT_DATA, 3,
			NULL,
		MENU_ITEM,
			MENU_STRING, "4",
			MENU_CLIENT_DATA, 4,
			NULL,
		MENU_ITEM,
			MENU_STRING, "5",
			MENU_CLIENT_DATA, 5,
			NULL,
		MENU_ITEM,
			MENU_STRING, "6",
			MENU_CLIENT_DATA, 6,
			NULL,
		MENU_ITEM,
			MENU_STRING, "7",
			MENU_CLIENT_DATA, 7,
			NULL,
		MENU_ITEM,
			MENU_STRING, "8",
			MENU_CLIENT_DATA, 8,
			NULL,
		MENU_ITEM,
			MENU_STRING, "9",
			MENU_CLIENT_DATA, 9,
			NULL,
		MENU_ITEM,
			MENU_STRING, "10",
			MENU_CLIENT_DATA, 10,
			NULL,
		MENU_ITEM,
			MENU_STRING, "11",
			MENU_CLIENT_DATA, 11,
			NULL,
		MENU_ITEM,
			MENU_STRING, "12",
			MENU_CLIENT_DATA, 12,
			NULL,
		MENU_ITEM,
			MENU_STRING, "13",
			MENU_CLIENT_DATA, 13,
			NULL,
		MENU_ITEM,
			MENU_STRING, "14",
			MENU_CLIENT_DATA, 14,
			NULL,
		MENU_ITEM,
			MENU_STRING, LGET("forever"),
			MENU_CLIENT_DATA, -1,
			NULL,
		XV_HELP_DATA, "cm:RepeatTimes",
		NULL);

	return(formenu);
}

static void
repeat_set_unit(m, mi)
        Menu m;
        Menu_item mi;
{
	ae_repeat_win_objects *r = (ae_repeat_win_objects *)xv_get(m,
						XV_KEY_DATA, INSTANCE);

        xv_set(r->repeatunitmessage,
		PANEL_LABEL_STRING, (char*)xv_get(mi, MENU_STRING),
		NULL);
	xv_set(m, MENU_NOTIFY_STATUS, XV_ERROR, NULL);
}

static Xv_opaque
ae_make_repeatunit_menu(ae_repeat_win_objects *ip)
{
        Menu menu;

        if (ip==NULL) return(NULL);

        menu = menu_create(
		XV_KEY_DATA, INSTANCE, ip,
                MENU_NOTIFY_PROC, repeat_set_unit,
                MENU_NOTIFY_STATUS, XV_ERROR,
                MENU_ITEM,
                        MENU_STRING,  periodstr[everyNthDay],
                        MENU_VALUE, 7,
                        0,
                MENU_ITEM,
                        MENU_STRING,  periodstr[everyNthWeek],
                        MENU_VALUE, 8,
                        0,
                MENU_ITEM,
                        MENU_STRING,  periodstr[everyNthMonth],
                        MENU_VALUE, 9,
                        0,
		XV_HELP_DATA, "cm:RepeatUnitMenu",
                0);

        return(menu);
}

extern void
ae_make_menus()
{
	xv_set(Ae_window->start_button, PANEL_ITEM_MENU,
			ae_make_start_menu(Ae_window), NULL);
	xv_set(Ae_window->stop_button, PANEL_ITEM_MENU,
			ae_make_stop_menu(Ae_window), NULL);
	xv_set(Ae_window->repeat_button, PANEL_ITEM_MENU,
			ae_make_repeat_menu(Ae_window), NULL);
	xv_set(Ae_window->for_button, PANEL_ITEM_MENU,
			ae_make_for_menu(Ae_window), NULL);
	xv_set(Ae_repeat_win->repeat_menu, PANEL_ITEM_MENU,
			ae_make_repeatunit_menu(Ae_repeat_win), NULL);
}

extern void
save_appt(char *appt)
{
	FILE *fd;

	if (appt == NULL)
		return;

	if ((fd = fopen(file, "w")) == NULL) {
		if (debug)
			perror(file);
	} else {
		fprintf(fd, "%s", appt);
		fclose(fd);
	}
}

/*
 * Notify callback function for `attach_button'.
 *
 * Get appt info from ui objects and either write it to a file or
 * send it through tooltalk to the requestor.  And quit.
 */
void
attach_proc(Panel_item item, Event *event)
{
	ae_window_objects *ip = (ae_window_objects *) xv_get(item,
					XV_KEY_DATA, INSTANCE);
	FILE	*fd;
	char	*appt = NULL;
	int	lastmsg;

	if (debug)
		fprintf(stderr, "changed_proc called\n");

	if ((appt = get_appt()) == NULL)
		xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
	else {
		lastmsg = (xv_get(ip->window, FRAME_CMD_PIN_STATE)
				== FRAME_CMD_PIN_IN) ? 0 : 1;

		if (tt_flag)
			ae_tt_send_appt(appt, lastmsg);
		else
			save_appt(appt);

		if (!lastmsg) {
			if (lastappt)
				free(lastappt);
			lastappt = appt;
			data_changed = FALSE;
		} else if (!tt_flag)
			timeout_quit();
	}
}

/*
 * Notify callback function for `reset_button'.
 */
void
reset_proc(Panel_item item, Event *event)
{
	ae_window_objects *ip = (ae_window_objects *) xv_get(item,
					XV_KEY_DATA, INSTANCE);

	data_changed = TRUE;
	set_default_values(ip);
	xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
}

/*
 * Drop callback function for `droptarget'.
 */
void
drop_on(Xv_opaque item, Event *event, GDD_DROP_INFO *drop_info)
{
	ae_window_objects *ip = (ae_window_objects *) xv_get(item,
					XV_KEY_DATA, INSTANCE);
	char filename[15];
	int result;

	if (lhost == NULL) {
		lhost = (char *)calloc(1, MAXHOSTNAMELEN);
#ifdef SVR4
		sysinfo(SI_HOSTNAME, lhost, MAXHOSTNAMELEN);
#else
		gethostname(lhost, MAXHOSTNAMELEN);
#endif
	}

	if ((drop_info->source_host &&
	     strcmp(drop_info->source_host, lhost) == 0) &&
	     drop_info->filename)

		/* get data from file on local host */
		result = load_proc(ip, drop_info->filename, TRUE);

	else if (drop_info->data) {
		result = load_data(drop_info->data, drop_info->length, TRUE);
	}

	if (result != -1)
		data_changed = TRUE;
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
		appt = get_appt();
		drop_info->app_name = strdup("ae");
		drop_info->data_label = NULL;
		drop_info->data = appt;
		drop_info->length = appt ? cm_strlen(appt) : 0;

		xv_set(item, PANEL_CLIENT_DATA, appt, NULL);
		break;

	case GDD_DRAG_COMPLETED:
		appt = (char *)xv_get(item, PANEL_CLIENT_DATA);
		if (appt)
			free(appt);
		break;

	}
}

/*
 * Notify callback for date_textfield.
 */
extern Panel_setting
date_notify(item, event)
	Panel_item item; Event *event;
{
	char *str = (char *)xv_get(item, PANEL_VALUE);

	if (str == NULL || cm_strlen(str) == 0)
		datefilled = FALSE;
	else
		datefilled = TRUE;

	set_dnd_target();

	data_changed = TRUE;
	return(panel_text_notify(item, event));
}

void
changed_proc(Panel_item item, int value, Event *event)
{
	data_changed = TRUE;
}

Panel_setting
changed_text_proc(Panel_item item, Event *event)
{
	data_changed = TRUE;
	return(panel_text_notify(item, event));
}

/*
 * notify callback for start_textfield.
 */
Panel_setting
start_notify(Panel_item item, Event *event)
{
	char *str = (char *)xv_get(item, PANEL_VALUE);

	if (str == NULL || cm_strlen(str) == 0)
		startfilled = FALSE;
	else
		startfilled = TRUE;

	set_dnd_target();

	data_changed = TRUE;
	return(panel_text_notify(item, event));
}

/*
 * notify callback for stop_textfield.
 */
Panel_setting
stop_notify(Panel_item item, Event *event)
{
	data_changed = TRUE;
	return(panel_text_notify(item, event));
}

/*
 * notify callback for what_textfields.
 */
Panel_setting
what_notify(Panel_item item, Event *event)
{
	char *str1 = (char *)xv_get(Ae_window->what_textfield1, PANEL_VALUE);
	char *str2 = (char *)xv_get(Ae_window->what_textfield2, PANEL_VALUE);
	char *str3 = (char *)xv_get(Ae_window->what_textfield3, PANEL_VALUE);
	char *str4 = (char *)xv_get(Ae_window->what_textfield4, PANEL_VALUE);

	if (str1 && cm_strlen(str1) > 0)
		whatfilled = TRUE;
	else if (str2 && cm_strlen(str2) > 0)
		whatfilled = TRUE;
	else if (str3 && cm_strlen(str3) > 0)
		whatfilled = TRUE;
	else if (str4 && cm_strlen(str4) > 0)
		whatfilled = TRUE;
	else
		whatfilled = FALSE;

	set_dnd_target();

	data_changed = TRUE;
	return(panel_text_notify(item, event));
}

static void
get_24hr_value(char *new_value, char *panel_value, int ampm)
{
	char	*ptr;
	int	hr;

	if (panel_value == NULL || *panel_value == '\0') {
		*new_value = '\0';
		return;
	}

	ptr = strchr(panel_value, ':');
	if (ptr != NULL)
		*ptr = '\0';

	hr = atoi(panel_value);
	if ((ampm && hr < 12) || (!ampm && hr == 12))
		hr += 12;

	sprintf(new_value, "%02d%s", (hr == 24 ? 0 : hr), (ptr ? ++ptr : "00"));
}

static void
get_12hr_value(char *new_value, char *panel_value, int *ampm)
{
	int	num, hr, minute;

	if (panel_value == NULL || *panel_value == '\0') {
		*new_value = '\0';
		return;
	}

	num = atoi(panel_value);
	hr = num / 100;
	minute = num % 100;
	*ampm = !adjust_hour(&hr);

	sprintf(new_value, "%2d:%02d", hr, minute);
}

extern void
update_props()
{
	int		ampm;
	char		*panel_value, *datemsg;
	char		new_value[20], m[3], d[3], y[5];
	Rc_value	oldrc, *rc = NULL;
	int		new_disp = FALSE;
	int		new_range = FALSE;
	int		new_format = FALSE;
	extern		char *separator[];
	
	if ((rc = (Rc_value *)xv_get(Ae_window->window, WIN_CLIENT_DATA))
	    == NULL)
		return;

	oldrc = *rc;

	/* read the property file again */
	if (get_rc_value())
		return;
	else
		rc = (Rc_value *)xv_get(Ae_window->window, WIN_CLIENT_DATA);

	if (rc->hour24 != oldrc.hour24) {
		new_disp = TRUE;

		if (rc->hour24) {
			ampm = (int)xv_get(Ae_window->start_ampm, PANEL_VALUE);
			panel_value = (char *)xv_get(Ae_window->start_textfield,
					PANEL_VALUE);
			get_24hr_value(new_value, panel_value, ampm);
			xv_set(Ae_window->start_ampm, PANEL_VALUE, 0, XV_SHOW,
				FALSE, NULL);
			xv_set(Ae_window->start_textfield, PANEL_VALUE,
				new_value, PANEL_VALUE_DISPLAY_LENGTH, 17,
				NULL);

			ampm = (int)xv_get(Ae_window->stop_ampm, PANEL_VALUE);
			panel_value = (char *)xv_get(Ae_window->stop_textfield,
					PANEL_VALUE);
			get_24hr_value(new_value, panel_value, ampm);
			xv_set(Ae_window->stop_ampm, PANEL_VALUE, 0, XV_SHOW,
				FALSE, NULL);
			xv_set(Ae_window->stop_textfield, PANEL_VALUE,
				new_value, PANEL_VALUE_DISPLAY_LENGTH, 17,
				NULL);
		} else {
			panel_value = (char *)xv_get(Ae_window->start_textfield,
					PANEL_VALUE);
			get_12hr_value(new_value, panel_value, &ampm);
			xv_set(Ae_window->start_textfield, PANEL_VALUE,
				new_value, PANEL_VALUE_DISPLAY_LENGTH, 8, NULL);
			xv_set(Ae_window->start_ampm, PANEL_VALUE, ampm,
				XV_SHOW, TRUE, NULL);

			panel_value = (char *)xv_get(Ae_window->stop_textfield,
					PANEL_VALUE);
			get_12hr_value(new_value, panel_value, &ampm);
			xv_set(Ae_window->stop_textfield, PANEL_VALUE,
				new_value, PANEL_VALUE_DISPLAY_LENGTH, 8, NULL);
			xv_set(Ae_window->stop_ampm, PANEL_VALUE, ampm,
				XV_SHOW, TRUE, NULL);
		}
	}

	if (rc->daybegin != oldrc.daybegin || rc->dayend != oldrc.dayend)
		new_range = TRUE;

	if (new_disp || new_range) {
		xv_set(Ae_window->start_button, PANEL_ITEM_MENU,
			ae_make_start_menu(Ae_window), NULL);
		xv_set(Ae_window->stop_button, PANEL_ITEM_MENU,
			ae_make_stop_menu(Ae_window), NULL);
	}

	if (rc->order != oldrc.order || rc->sep != oldrc.sep) {
		datemsg = get_datemsg(rc->order, rc->sep);
		xv_set(Ae_window->date_message, PANEL_LABEL_STRING, datemsg,
				NULL);
		free(datemsg);

		panel_value = (char *)xv_get(Ae_window->date_textfield,
					PANEL_VALUE);
		parse_date(oldrc.order, oldrc.sep, panel_value, m, d, y);
		switch (rc->order) {
		case Order_DMY:
			sprintf(new_value, "%s%s%s%s%s", d, separator[rc->sep], m,
				separator[rc->sep], y);
			break;
		case Order_YMD:
			sprintf(new_value, "%s%s%s%s%s", y, separator[rc->sep], m,
				separator[rc->sep], d);
			break;
		case Order_MDY:
			sprintf(new_value, "%s%s%s%s%s", m, separator[rc->sep], d,
				separator[rc->sep], y);
			break;
		}

		xv_set(Ae_window->date_textfield, PANEL_VALUE, new_value, NULL);
	}
}

void
repeat_win_done_proc(Frame frame)
{
	xv_set(frame, XV_SHOW, FALSE, NULL);
}

void
apply_proc(Panel_item item, Event *event)
{
	ae_repeat_win_objects *repeatui = (ae_repeat_win_objects *)xv_get(item,
						XV_KEY_DATA, INSTANCE);
	ae_window_objects *ip = (ae_window_objects *)xv_get(
			(Xv_object)xv_get(repeatui->repeat_win, XV_OWNER),
			XV_KEY_DATA, INSTANCE);
	int val=0;
	int unit;
	char buf[80];

	if ((val = (int)atoi((char*)xv_get(repeatui->repeatunit, PANEL_VALUE))) < 1) {
		notice_prompt(repeatui->repeat_win, (Event *)NULL,
			NOTICE_MESSAGE_STRING,
			MGET("Invalid Entry"),
			NOTICE_BUTTON_YES,
			LGET("Continue") ,
		NULL);
	}
        
	unit = repeatstr_to_interval(
		(char*)xv_get(repeatui->repeatunitmessage, PANEL_LABEL_STRING));
	sprintf(buf, "%s %d %s", MGET("Every"), val, periodstr[unit]);
	/* use repeat_button's PANEL_CLIENT_DATA to hold the nth value */
	xv_set(ip->repeat_button, PANEL_CLIENT_DATA, val, NULL);
	xv_set(ip->repeat_message, PANEL_LABEL_STRING, buf,
		PANEL_CLIENT_DATA, unit,
		NULL);
	xv_set(ip->for_textfield, PANEL_VALUE, repeatval[unit], 0);
        xv_set(ip->days_message, PANEL_LABEL_STRING, repeatstr[unit], 0);

	if (unit) {
		xv_set(ip->for_button, PANEL_INACTIVE, FALSE, NULL);
		xv_set(ip->for_textfield, PANEL_INACTIVE, FALSE,
			PANEL_VALUE, repeatval[unit],
			NULL);
		xv_set(ip->days_message, PANEL_INACTIVE, FALSE,
			PANEL_LABEL_STRING, repeatstr[unit],
			NULL);
	} else {
		xv_set(ip->for_button, PANEL_INACTIVE, TRUE, NULL);
		xv_set(ip->for_textfield, PANEL_INACTIVE, TRUE,
			PANEL_VALUE, repeatval[unit],
			NULL);
		xv_set(ip->days_message, PANEL_INACTIVE, TRUE,
			PANEL_LABEL_STRING, repeatstr[unit],
			NULL);
	}

	data_changed = TRUE;
}
