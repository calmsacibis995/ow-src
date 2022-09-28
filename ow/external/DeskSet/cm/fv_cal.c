#ifndef lint
static  char sccsid[] = "@(#)fv_cal.c 3.27 94/09/13 Copyr 1991 Sun Microsystems, Inc.";
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <rpc/rpc.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/canvas.h>
#include <xview/textsw.h>
#include <xview/scrollbar.h>
#include <xview/notice.h>
#include <xview/font.h>
#include <ds_listbx.h>
#include "util.h"
#include "editor.h"
#include "appt.h"
#include "table.h"
#include "graphics.h"
#include "calendar.h"
#include "timeops.h"
#include "browser.h"
#include "datefield.h"
#include "props.h"
#include "gettext.h"
#include "alist.h"
#include "todo.h"
#include "misc.h"

#ifdef __STDC__
#include <stdarg.h>     /* for variable length declarations */
#endif /* __STDC__ */

#ifdef SVR4
#include <sys/param.h>
#endif "SVR4"

#define NOTICE_CANCEL	2

extern int cm_get_boolean_property();

/*
 * Warn the user about an error.
 */
/* VARARGS1 */
int
#ifdef __STDC__
error_alert(Frame frame, ...)
#else /*  __STDC__ */
error_alert(frame, va_alist)
	Frame		frame;
	va_dcl
#endif /*  __STDC__ */
{
	va_list         args;
	Event           ie;
	char		*p;
	int	status, i;
	char	*array[10];

	i = 0;
#ifdef __STDC__
	va_start(args, frame);
#else /*  __STDC__ */
	va_start(args);
#endif  /*  __STDC__ */
    	while ((p = va_arg(args, char *)) != (char *) 0) 
		array[i++] = p;
	va_end(args);
	array[i] = (char *) 0;
	status = notice_prompt(frame,
		&ie,
		NOTICE_MESSAGE_STRINGS_ARRAY_PTR, array,
		NOTICE_BUTTON_YES,  LGET("Continue") ,
		0);
	switch (status) {
		case NOTICE_YES: return;
		case NOTICE_TRIGGERED: return; 
		case NOTICE_FAILED: alert_failed( EGET("Could not put up alert to warn user of the following condition:\n"), array, "");
			return;
	}
}
static Boolean
get_appointment(a, c)
	Appt **a;
	Calendar	*c;
{
	/*
	 * Get an appointment struct to schedule a dropped appointment message
	 */
	if ((*a = (Appt *) make_appt()) == NULL) {
		error_alert(calendar->frame,  EGET("Unable to allocate appointment") ,
			0);
		return(NULL);
	}
	(*a)->author = cm_strdup((char *)c->user);
	return 1;
}

static void
schedule_appt(c, a)
	Calendar	*c;
	Appt		*a;

{
	Appt	*na=NULL;
	char	buf[100];
	int start_ind, end_ind;
	extern struct drawind *index_list; 
	Browser *b = (Browser*)c->browser;
	Editor *e = (Editor*)c->editor;
	Props *p = (Props*)c->properties;
	Todo *t = (Todo*)c->todo;
	Alist *al = (Alist*)c->alist;
	int lastwk, wk, alert_res;
	Stat tbl_stat;
	char	date_buf[100];

	index_list = NULL;

	/* if the repeating type is "Monthly by weekday",
	 * fix the nth value, i.e., the week of month
	 * the appt is in
	 */
	if (a->period.period == nthWeekday) {
		lastwk = weekofmonth(a->appt_id.tick, &wk);
		if (wk != a->period.nth && a->period.nth != -1) {
			if (lastwk && wk == 4) {
				alert_res = notice_prompt(c->frame, (Event *)NULL,
					NOTICE_MESSAGE_STRINGS,
					MGET("Would you like to schedule this appointment"),
					MGET("as the Last Week of the Month or the"),
					MGET("4th Week of the Month?"),
					NULL,
					NOTICE_BUTTON_YES, LGET("Last Week"),
					NOTICE_BUTTON_NO, LGET("4th Week"),
					NOTICE_BUTTON, LGET("Cancel"), NOTICE_CANCEL,
					NULL);
				if (alert_res == NOTICE_CANCEL) {
					destroy_appt(a);
					return;
				} else if (alert_res == NOTICE_YES)
					a->period.nth = -1;
				else
					a->period.nth = wk;
			} else if (wk == 5)
				a->period.nth = -1;
			else
				a->period.nth = wk;
		}
	}

	/* if the user is viewing other people's calendar,
	 * prompt and ask if he really wants to schedule
	 * the appointment into the viewing calendar
	 */
	if (strcmp(c->calname, c->view->current_calendar)) {
		sprintf(buf,  MGET("calendar %s"), c->view->current_calendar);
		alert_res = notice_prompt(c->frame, (Event *)NULL,
			NOTICE_MESSAGE_STRINGS,
			MGET("The appointment will be scheduled in"),
			buf,
			MGET("Do you still want to schedule it?"),
			NULL,
			NOTICE_BUTTON_YES, LGET("Schedule"),
			NOTICE_BUTTON_NO, LGET("Cancel"),
			NULL);
		if (alert_res == NOTICE_NO) {
			destroy_appt(a);
			return;
		}
	}

	/* insert appointment into the viewing calendar */
	if ((tbl_stat = table_insert(c->view->current_calendar, a, &na))
		!= status_ok)
	{
		if (tbl_stat == status_denied)
			sprintf(buf, EGET("Insert Access Denied: Appointment Not Inserted."));
		else
			sprintf(buf, EGET("Insertion Failed: Appointment Not Inserted."));
		notice_prompt(c->frame, (Event *)NULL,
			NOTICE_MESSAGE_STRINGS,
				buf,
				NULL,
			NOTICE_BUTTON_YES, LGET("Continue"),
			NULL);
		destroy_appt(a); destroy_appt(na);
		return;
	} else {
		format_tick(a->appt_id.tick, p->ordering_VAL, p->separator_VAL,
				date_buf);
		sprintf(buf,  MGET("Appointment scheduled:  %s\n") , date_buf);
		xv_set(c->frame, FRAME_LEFT_FOOTER, buf, 0);
		if (xv_get(c->frame, FRAME_CLOSED)) 
			notice_prompt(c->frame, (Event *)NULL,
			NOTICE_MESSAGE_STRINGS,
				buf,
				NULL,
			NOTICE_BUTTON_YES, LGET("Continue"),
			NULL);
	}

	paint_canvas(c, NULL);
	if (browser_showing(b) &&
		list_is_selected(b->box, c->view->current_calendar)  
		&& showing_browser(b, na->appt_id.tick)) {
		b->add_to_array = true;
		mb_update_segs(b, na->appt_id.tick, na->duration, &start_ind, &end_ind);
		mb_add_index(start_ind, end_ind);
		if (index_list != NULL) mb_redisplay(b);
                list_items_selected(b->box, mb_update_busyicon);
        }
	if (editor_showing(e))
		add_times(e->apptbox);
	if (na != NULL) {
                if (na->tag->tag == appointment && alist_showing(c->alist)) {
			al->changed = true;
        		a_create_alist(c, al->glance);
		}
                if (na->tag->tag == toDo && todo_showing(c->todo)) {
			t->changed = true;
        		t_create_todolist(c, t->glance);
		}
        }
	reset_alarm(c);
	destroy_appt(na); destroy_appt(a);
}

static void
attach_default_reminders(a)
	Appt *a;
{
	char buf[35];
	Attribute *attr;
	int secs;
	Props *p = (Props*)calendar->properties;

	cal_update_props();
	/* Attach default reminders to appt */
	if ( cm_get_boolean_property(property_names[CP_BEEPON]) ) {
		/* allocate space for attr and value */
		attr = make_attr();
		attr->attr = cm_strdup("bp");
		secs = units_to_secs(p->beepunit_VAL, p->beepadvance_VAL);
		sprintf(buf, "%d", secs);
		attr->value = cm_strdup(buf);
		attr->next = a->attr;
		a->attr = attr;
	}
	if ( cm_get_boolean_property(property_names[CP_FLASHON]) ) {
		attr = make_attr();
		attr->attr = cm_strdup("fl");
		secs = units_to_secs(p->flashunit_VAL, p->flashadvance_VAL);
		sprintf(buf, "%d", secs);
		attr->value = cm_strdup(buf);
		attr->next = a->attr;
		a->attr = attr;
	}

	if ( cm_get_boolean_property(property_names[CP_OPENON]) ) {
		attr = make_attr();
		attr->attr = cm_strdup("op");
		secs = units_to_secs(p->openunit_VAL, p->openadvance_VAL);
		sprintf(buf, "%d", secs);
		attr->value = cm_strdup(buf);
		attr->next = a->attr;
		a->attr = attr;
	}

	if ( cm_get_boolean_property(property_names[CP_MAILON]) ) {
		attr = make_attr();
		attr->attr = cm_strdup("ml");
		secs = units_to_secs(p->mailunit_VAL, p->mailadvance_VAL);
		sprintf(buf, "%d", secs);
		attr->value = cm_strdup(buf);
		attr->clientdata = (Buffer)malloc(sizeof(char)*cm_strlen(p->mailto_VAL)+1);
		cm_strcpy(attr->clientdata, p->mailto_VAL);
		attr->next = a->attr;
		a->attr = attr;
	}
}

extern int 
drag_load_proc(filename, newappt) 
        char *filename;
	Appt **newappt;
{ 
        register FILE    *f=NULL; 
	register short	nwhat_lines = 0;
	char	*temp, *temp2;
	int	what_len = 0, len;
	Boolean	set_date=false, set_time=false;
        char    line[200];
	char	date_buf[100];
	char	tmp_buf[100];
	char	*ptr;
	int	i;
	Appt    *a;
	Tick 	enddate = 0;
	Boolean set_ntimes = false, eshow = false;
	Editor *e = (Editor*)calendar->editor;

	eshow = editor_showing(e);
	if (newappt != NULL)
		*newappt = NULL;

	/*
	 * We've had a file dropped on us.  Parse the file to schedule
	 * appointments. 
	 */
	date_buf[0] = '\0';
	if (filename != NULL && *filename != '\0') {
        	if ((f = fopen(filename, "r")) == NULL) {
			error_alert(calendar->frame,  EGET("Unable to open file.") ,
			strerror(errno), 0);
			return(-1); 
		}

		if (newappt && eshow)
			xv_set(e->frame, FRAME_BUSY, TRUE, NULL);
		else
			xv_set(calendar->frame, FRAME_BUSY, TRUE, 0);

		if (get_appointment(&a, calendar) == NULL) {
			fclose(f);
			if (newappt && eshow)
				xv_set(e->frame, FRAME_BUSY, FALSE, NULL);
			else
				xv_set(calendar->frame, FRAME_BUSY, FALSE, 0);
			return(-1);
		}
		a->duration = 0;

		while (fgets (line, sizeof(line), f)) {
		/*
		 * Skip blank lines and lines without leading whitespace
		 */
			if ((*line != ' ' && *line != '\t') || blank_buf(line)) {
				nwhat_lines = 0;
				continue;
			}

			/*
			 * Get lower case version of first word on line
			 */
			if ((temp = (char *) strtok(line, " \t")) == NULL)
				continue;
			
			/*
			 * Check first word
			 */
			if (strcasecmp(temp,  "date:" ) == 0) {
				if (set_date) {
				/*
				 * We've reached another appointment.
				 * Schedule the current and reset.
				 */
					if (newappt) {
						/* only care for the 1st one */
						fclose(f);
						goto done;
					}

					if (!set_time && enddate != 0)
						goto ERROR_EXIT;

					if ((a->period.period == single &&
					     a->ntimes != 0) ||
					    (a->period.period != single &&
					     a->ntimes == 0))
						goto ERROR_EXIT;

					if (!set_time && blank_buf(a->what))
						goto ERROR_EXIT;

					if (!set_time)
						a->tag->showtime = false;

					if (enddate)
						if ((a->duration = enddate -
						    a->appt_id.tick) < 0)
							a->duration = enddate +
								daysec -
								a->appt_id.tick;

					attach_default_reminders(a);
					schedule_appt(calendar, a);
					if (get_appointment(&a, calendar) == NULL){
						fclose(f);
						if (newappt && eshow)
							xv_set(e->frame,
								FRAME_BUSY,
								FALSE, NULL);
						else
							xv_set(calendar->frame,
								FRAME_BUSY,
								FALSE, 0);
						return(-1);
					}
					a->duration = 0;
					set_date = set_time = false;
					*date_buf = '\0';

					/* other part has used strtok */
					/* so we need to do it again */
					line[cm_strlen(line)] = ' ';
					temp = (char *)strtok(line, " \t");
				}
				temp = (char *)strtok(NULL, "\n");
				if (temp != NULL) {
#if 0
					cm_strcat(date_buf, temp);
#endif
					if ((a->appt_id.tick =
					     cm_getdate(temp, NULL)) < 0)
						goto ERROR_EXIT;
					/*
 					 *  See comment above regarding bugid
 					 *  # 1128469.
 					 */
 					format_tick(a->appt_id.tick, 
 						Order_MDY,
						Separator_Slash, 
 						date_buf);
					set_date=true;
				} else
					/* don't allow null date */
					goto ERROR_EXIT;
				nwhat_lines = 0;
	
			} else if (strcasecmp(temp,  "time:" ) == 0 ||
				   strcasecmp(temp,  "start:" ) == 0 ||
				   strcasecmp(temp,  "from:" ) == 0) {
				if (!set_date)
					goto ERROR_EXIT;
				temp = (char *)strtok(NULL, "\n");
				if (temp != NULL && !blank_buf(temp)) {
					cm_strcpy(tmp_buf, date_buf);
					cm_strcat(tmp_buf, " ");
					cm_strcat(tmp_buf, temp);
					if (temp[len = cm_strlen(temp)-1] == 'a' 
						|| temp[len] == 'p')
						cm_strcat(tmp_buf, "m");
					if ((a->appt_id.tick =
					     cm_getdate (tmp_buf, NULL)) < 0)
						goto ERROR_EXIT;
					set_time=true;
				}
				nwhat_lines = 0;

			} else if (strcasecmp(temp,  "end:" ) == 0 ||
				   strcasecmp(temp,  "until:" ) == 0 ||
				   strcasecmp(temp,  "stop:" ) == 0 ||
				   strcasecmp(temp,  "to:" ) == 0) {

				temp = (char *)strtok(NULL, "\n");
				if (temp != NULL && !blank_buf(temp)) {
					cm_strcpy(tmp_buf, date_buf);
					cm_strcat(tmp_buf, " ");
					cm_strcat(tmp_buf, temp);
					if (temp[len = cm_strlen(temp)-1] == 'a' 
						|| temp[len] == 'p')
						cm_strcat(tmp_buf, "m");
					enddate = cm_getdate(tmp_buf, NULL); 
					if (enddate < 0)
						goto ERROR_EXIT;
				}
				nwhat_lines = 0;

			} else if (strcasecmp(temp,  "duration:" )==0) {
				temp = (char *)strtok(NULL, "\n");
				if (temp != NULL) {
					if ((a->duration = getduration(temp)) < 0)
						goto ERROR_EXIT;
				}
				nwhat_lines = 0;
	
			} else  if (strcasecmp(temp,  "what:" )==0) {
				if ((temp = (char *)strtok(NULL,"\n")) != NULL)
					while (*temp == ' ' || *temp == '\t')
						temp++;
	
				if (a->what != NULL)
					free(a->what);
	
				if (temp == NULL)
					what_len = 2;
				else
					what_len = cm_strlen(temp) + 2;
				a->what = (Buffer) ckalloc(what_len);
				if (temp != NULL)
					cm_strcpy (a->what, temp);
				else 
					a->what[0] = '\0';
				cm_strcat(a->what, "\n");
				nwhat_lines = 1;

			} else if (strcasecmp(temp, "repeat:") == 0) {
				if ((temp=(char *)strtok(NULL,"\n")) != NULL) {
					while (*temp == ' ' || *temp == '\t')
						temp++;

					ptr = &temp[cm_strlen(temp)-1];
					while (*ptr && ptr > temp &&
						isspace(*ptr))
					{
						*ptr = '\0';
						ptr--;
					}
					a->period.period = (Interval)pstr_to_unitsCloc(
						temp, &(a->period.nth));

					if (a->period.period == -1) {
						ptr = strrchr(temp, ',');
						if (ptr)
							*ptr = '\0';
						else
							goto ERROR_EXIT;
						a->period.period = pstr_to_unitsCloc(
							temp, &(a->period.nth));
						if (a->period.period == -1)
							goto ERROR_EXIT;
					}

					if (a->period.period == nthWeekday &&
					    ptr++) {
						while (*ptr==' '||*ptr=='\t')
							ptr++;
						if (*ptr &&
						    !strncasecmp(ptr,"last",4))
							a->period.nth = -1;
						else
							a->period.nth = atoi(ptr);
					}
					if (set_ntimes == false)
						a->ntimes = atoi(
							repeatval[a->period.period]);
				}
				nwhat_lines = 0;

			} else if (strcasecmp(temp, "for:") == 0) {
				if ((temp=(char *)strtok(NULL,"\n")) != NULL) {
					while (*temp == ' ' || *temp == '\t')
						temp++;
					if (strcasecmp(temp, "forever"))
						a->ntimes = atoi(temp);
					else
						a->ntimes = CMFOREVER;
					set_ntimes = true;
				}
				nwhat_lines = 0;

			} else if (nwhat_lines > 0 && nwhat_lines < 5) {
				what_len += cm_strlen(temp) + 2;
				temp2 = (char *)strtok(NULL,"\n");
				if (temp2 != NULL)
					what_len += cm_strlen(temp2) + 2;
				a->what = (char *)realloc(a->what, what_len);
	
				cm_strcat (a->what, temp);
				if (temp2 != NULL) {
					cm_strcat (a->what, " ");
					cm_strcat (a->what, temp2);
					cm_strcat(a->what, "\n");
				}
				nwhat_lines++;
			}
		}

		fclose(f);
	}

done:
	if (!set_date || (!set_time && enddate != 0))
		goto ERROR_EXIT;
	
	if ((a->period.period == single && a->ntimes != 0) ||
	    (a->period.period != single && a->ntimes == 0))
		goto ERROR_EXIT;

	if (!set_time && blank_buf(a->what))
		goto ERROR_EXIT;

	if (!set_time)
		a->tag->showtime = false;

	if (enddate)
		if ((a->duration = enddate - a->appt_id.tick) < 0)
			a->duration = enddate + daysec - a->appt_id.tick;

	if (newappt) {
		*newappt = a;
		if (eshow)
			xv_set(e->frame, FRAME_BUSY, FALSE, NULL);
	} else {
		attach_default_reminders(a);
		schedule_appt(calendar, a);
		xv_set(calendar->frame, FRAME_BUSY, FALSE, 0);
	}
	
	return(0);

ERROR_EXIT:
	if (f != NULL) fclose(f);
	destroy_appt(a);
	if (newappt)
		error_alert(calendar->frame,
			EGET("Unable to display appointment.") ,
			EGET("Invalid appointment format.") , 0);
	else
		error_alert(calendar->frame,
			EGET("Unable to schedule appointment.") ,
			EGET("Invalid appointment format.") , 0);

	if (newappt && eshow)
		xv_set(e->frame, FRAME_BUSY, FALSE, NULL);
	else
		xv_set(calendar->frame, FRAME_BUSY, FALSE, 0);
	return(-1);
}


static
getduration(buffer)

	char	*buffer;

{
	register char	*p;
		 char	c;
		 int	n;

	/*
	 * Toss leading white space
	 */
	while (*buffer == ' ' || *buffer == '\t')
		buffer++;

	/*
	 * Get number
	 */
	p = buffer;
	while (isdigit(*p))
		p++;
	if (p == buffer)
		return(-1);
	c = *p;
	*p = '\0';
	n = atoi(buffer);
	*p = c;

	/*
	 * Toss white space between number and unit
	 */
	while (*p == ' ' || *p == '\t')
		p++;

	switch(*p) {

	case 'h':
	case 'H':
		n *= 3600;
		break;
	default:
		n *= 60;
		break;
	}

	return(n);
}

alert_failed(str1, array, str2)
	char	*str1, *str2;
	char	*array[];
{
	int	i = 0;
	(void) fprintf(stderr, str1);
	for (i = 0; array[i] != (char *) 0; i++)
		(void) fprintf(stderr, array[i]);
	(void) fprintf(stderr, str2);
}

