#ifndef lint
static  char sccsid[] = "@(#)ae.c 1.28 94/09/13 Copyr 1991 Sun Microsystems, Inc.";
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
#include <errno.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/font.h>
#include <xview/notice.h>
#include <gdd.h>
#include <time.h>
#include <unistd.h>
#include "gettext.h"
#include "misc.h"
#include "ae_ui.h"
#include "ae.h"
#include "ae_proc.h"
#include "ae_tt.h"

extern char	*cm_strdup(char *);

#define NULLSTRING	""
#define SHORTDISPLAY	8
#define LONGDISPLAY	17

ae_window_objects	*Ae_window;
ae_repeat_win_objects	*Ae_repeat_win;

int	data_changed;
int	debug = 0;
char	*file = 0;
int	tt_flag = FALSE;

/* properties we need */
#define RCDATEORDER	0
#define RCSEPARATOR	1
#define RCDEFAULTDISP	2
#define RCDAYBEGIN	3
#define RCDAYEND	4
#define DSDATEORDER	5
#define DSSEPARATOR	6
#define DSDEFAULTDISP	7
#define DSDAYBEGIN	8
#define DSDAYEND	9

char *prop_names[] = {
	"Calendar.DateOrdering",
	"Calendar.DateSeparator",
	"Calendar.DefaultDisplay",
	"Calendar.DayBegin",
	"Calendar.DayEnd",
	"deskset.calendar.DateOrdering",
	"deskset.calendar.DateSeparator",
	"deskset.calendar.DefaultDisplay",
	"deskset.calendar.DayBegin",
	"deskset.calendar.DayEnd",
	NULL
};

/*
 * get property values from .cm.rc or .desksetdefaults.
 */
extern int
get_rc_value()
{
	Rc_value *rcval = (Rc_value *)xv_get(Ae_window->window, WIN_CLIENT_DATA);
	int value;
	Pentry *headp, *pentries;

	if (rcval == NULL) {
		if ((rcval = calloc(1, sizeof(Rc_value))) == NULL)
			return -1;

		/* set default values */
		rcval->order = Order_MDY;
		rcval->sep = Separator_Slash;
		rcval->hour24 = FALSE;
		rcval->daybegin = DEFDAYBEGIN;
		rcval->dayend = DEFDAYEND;
	}

	headp = pentries = cm_get_resources();
	while (pentries != NULL) {
		if (!strcmp(pentries->property_name, prop_names[RCDATEORDER]) ||
		    !strcmp(pentries->property_name, prop_names[DSDATEORDER])) {
			value = atoi(pentries->property_value);
			if (value >= Order_MDY && value <= Order_YMD) 
				rcval->order = (Ordering_Type)value;
		} else if (!strcmp(pentries->property_name, prop_names[RCSEPARATOR]) ||
			!strcmp(pentries->property_name, prop_names[DSSEPARATOR])) {

			rcval->sep = (Separator_Type)atoi(pentries->property_value);

		} else if (!strcmp(pentries->property_name, prop_names[RCDEFAULTDISP]) ||
			!strcmp(pentries->property_name, prop_names[DSDEFAULTDISP])) {
			if (atoi(pentries->property_value))
				rcval->hour24 = TRUE;
			else
				rcval->hour24 = FALSE;
		} else if (!strcmp(pentries->property_name, prop_names[RCDAYBEGIN]) ||
			!strcmp(pentries->property_name, prop_names[DSDAYBEGIN])) {
			if (value = atoi(pentries->property_value))
				rcval->daybegin = value;
		} else if (!strcmp(pentries->property_name, prop_names[RCDAYEND]) ||
			!strcmp(pentries->property_name, prop_names[DSDAYEND])) {
			if (value = atoi(pentries->property_value))
				rcval->dayend = value;
		} else {
			pentries = pentries->next;
			continue;
		}
		pentries = pentries->next;
	}
	cm_free_resources(headp);

	xv_set(Ae_window->window, WIN_CLIENT_DATA, rcval, NULL);
	return 0;
}

static Miniappt *
get_appointment(ae_window_objects *ip)
{
	Miniappt *appt;

	if ((appt = (Miniappt *)calloc(1, sizeof(Miniappt))) == NULL)
		notice_prompt(ip->window, (Event *)NULL,
				NOTICE_MESSAGE_STRINGS,
				EGET("Unable to allocate memory"),
				NULL,
				NOTICE_BUTTON_YES, LGET("Continue"),
				NULL);

	return(appt);
}

static void
destroy_appt(Miniappt *appt)
{
	if (appt == NULL)
		return;

	if (appt->datestr != NULL)
		free(appt->datestr);
	if (appt->formattedstr != NULL)
		free(appt->formattedstr);
	if (appt->what1 != NULL)
		free(appt->what1);
	if (appt->what2 != NULL)
		free(appt->what2);
	if (appt->what3 != NULL)
		free(appt->what3);
	if (appt->what4 != NULL)
		free(appt->what4);

	free(appt);
}

static void
show_appt(ae_window_objects *ip, Miniappt *appt) 
{
	time_t		mytime;
	struct	tm	*mytimestruct;
	char		buff[100];
	int		ampm = 0;
	Rc_value	*rc;

	rc = (Rc_value *)xv_get(ip->window, WIN_CLIENT_DATA);

	xv_set(ip->date_textfield, PANEL_VALUE, appt->formattedstr, NULL);
	if (rc == NULL || rc->hour24 == FALSE) {
		if (appt->showstart) {
			ampm = !adjust_hour(&appt->starthr);
			sprintf(buff,"%2d:%02d", appt->starthr, appt->startmin);
			xv_set(ip->start_textfield, PANEL_VALUE, buff, NULL);
			xv_set(ip->start_ampm, PANEL_VALUE, ampm, XV_SHOW, TRUE,
			NULL);
		} else {
			xv_set(ip->start_textfield, PANEL_VALUE, NULLSTRING,
				NULL);
			xv_set(ip->start_ampm, PANEL_VALUE, 0, XV_SHOW, TRUE,
				NULL); 
		}
		if (appt->showstop) {
			ampm = !adjust_hour(&appt->endhr);
			sprintf(buff, "%2d:%02d", appt->endhr, appt->endmin);
			xv_set(ip->stop_textfield, PANEL_VALUE, buff, NULL);
			xv_set(ip->stop_ampm, PANEL_VALUE, ampm, XV_SHOW, TRUE,
			NULL);
		} else {
			xv_set(ip->stop_textfield, PANEL_VALUE, NULLSTRING,
				NULL);
			xv_set(ip->stop_ampm, PANEL_VALUE, 0, XV_SHOW, TRUE,
				NULL); 
		}
	} else {
		if (appt->showstart) {
			sprintf(buff,"%02d%02d", appt->starthr, appt->startmin);
			xv_set(ip->start_textfield, PANEL_VALUE, buff,
				PANEL_VALUE_DISPLAY_LENGTH, 17, NULL);
			xv_set(ip->start_ampm, XV_SHOW, FALSE, NULL);
		} else {
			xv_set(ip->start_textfield, PANEL_VALUE, NULLSTRING,
				PANEL_VALUE_DISPLAY_LENGTH, 17, NULL);
			xv_set(ip->start_ampm, PANEL_VALUE, 0, XV_SHOW, FALSE,
				NULL); 
		}
		if (appt->showstop) {
			sprintf(buff, "%02d%02d", appt->endhr, appt->endmin);
			xv_set(ip->stop_textfield, PANEL_VALUE, buff,
				PANEL_VALUE_DISPLAY_LENGTH, 17, NULL);
			xv_set(ip->stop_ampm, XV_SHOW, FALSE, NULL);
		} else {
			xv_set(ip->stop_textfield, PANEL_VALUE, NULLSTRING,
				PANEL_VALUE_DISPLAY_LENGTH, 17, NULL);
			xv_set(ip->stop_ampm, PANEL_VALUE, 0, XV_SHOW, FALSE,
				NULL); 
		}
	}

	xv_set(ip->repeat_message, PANEL_LABEL_STRING,
		period_to_str(appt->repeat, appt->nth),
		PANEL_CLIENT_DATA, appt->repeat,
		NULL);
	xv_set(ip->repeat_button, PANEL_CLIENT_DATA, appt->nth, NULL);

	if (appt->repeat) {
		if (appt->ntimes == -1)
			sprintf(buff, "%s", LGET("forever"));
		else
			sprintf(buff, "%d", appt->ntimes);
		xv_set(ip->for_button, PANEL_INACTIVE, FALSE, NULL);
		xv_set(ip->for_textfield, PANEL_INACTIVE, FALSE,
			PANEL_VALUE, buff,
			XV_SHOW, TRUE,
			NULL);
		xv_set(ip->days_message, PANEL_INACTIVE, FALSE,
			PANEL_LABEL_STRING, repeatstr[appt->repeat],
			XV_SHOW, TRUE,
			NULL);
	} else {
		xv_set(ip->for_button, PANEL_INACTIVE, TRUE, NULL);
		xv_set(ip->for_textfield, PANEL_INACTIVE, TRUE,
			PANEL_VALUE, repeatval[appt->repeat], NULL);
		xv_set(ip->days_message, PANEL_INACTIVE, TRUE,
			PANEL_LABEL_STRING, repeatstr[appt->repeat], NULL);
	}

	xv_set(ip->what_textfield1, PANEL_VALUE,
		(appt->what1 ? appt->what1 : ""), NULL);
	xv_set(ip->what_textfield2, PANEL_VALUE,
		(appt->what2 ? appt->what2 : ""), NULL);
	xv_set(ip->what_textfield3, PANEL_VALUE,
		(appt->what3 ? appt->what3 : ""), NULL);
	xv_set(ip->what_textfield4, PANEL_VALUE,
		(appt->what4 ? appt->what4 : ""), NULL);
	xv_set(ip->controls,
		PANEL_CARET_ITEM, Ae_window->what_textfield1, NULL);

	check_dnd_fullness(appt);
}

/*
 * load appointment from file
 */
extern int 
load_proc(ae_window_objects *ip, char *filename, int fromdnd) 
{
	FILE	*f=NULL;
	short	nwhat_lines = 0;
	char	*temp, *temp2;
	int	what_len = 0;
	int	set_date = FALSE, set_time = FALSE, findwhat = FALSE;
        char    line[200];
	char	date_buf[100];
	char	tmp_buf[100];
	char	*ptr, **ptrptr;
	Miniappt *a;
	long 	enddate = 0, atick;
	Rc_value *rc = NULL;
	struct tm *tm;
	int	set_ntimes = FALSE;

	rc = (Rc_value *)xv_get(ip->window, WIN_CLIENT_DATA);

	date_buf[0] = '\0';
	if (filename != NULL && *filename != '\0') {
        	if ((f = fopen(filename, "r")) == NULL) {
			notice_prompt(ip->window, (Event *)NULL,
				NOTICE_MESSAGE_STRINGS,
				EGET("Unable to open file."),
				strerror(errno),
				NULL,
				NOTICE_BUTTON_YES, LGET("Continue"),
				NULL);
			return(-1); 
		}

		xv_set(ip->window, FRAME_BUSY, TRUE, 0);
		if ((a = get_appointment(ip)) == NULL) {
			fclose(f);
			xv_set(ip->window, FRAME_BUSY, FALSE, 0);
			return(-1);
		}

		while (fgets (line, sizeof(line), f)) {
			/*
			 * Skip blank lines and lines without leading whitespace
			 */
			if ((*line != ' '&& *line != '\t')||blank_buf(line)) {
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
				 * Stop, we only care for the first one.
				 */
					fclose(f);
					goto done;
				}
				temp = (char *)strtok(NULL, "\n");
				if (temp != NULL && !blank_buf(temp)) {
					atick = cm_getdate(temp, NULL);
					if (atick < 0)
						goto ERROR_EXIT;

					format_tick(atick,
					    (rc ? rc->order : Order_MDY),
					    (rc ? rc->sep : Separator_Slash),
					    date_buf);
					a->datestr = cm_strdup(temp);
					a->formattedstr = cm_strdup(date_buf);
					set_date = TRUE;
				} else
					goto ERROR_EXIT;
				nwhat_lines = 0;
	
			} else if (strcasecmp(temp,  "time:" ) == 0 ||
				   strcasecmp(temp,  "start:" ) == 0 ||
				   strcasecmp(temp,  "from:" ) == 0) {

				if (!set_date)
					goto ERROR_EXIT;

				temp = (char *)strtok(NULL, "\n");
				if (temp != NULL && !blank_buf(temp)) {
					cm_strcpy(tmp_buf, a->datestr);
					cm_strcat(tmp_buf, " ");
					cm_strcat(tmp_buf, temp);
					atick = cm_getdate(tmp_buf, NULL);
					if (atick < 0)
						goto ERROR_EXIT;
					else {
						tm = localtime(&atick);
						a->starthr = tm->tm_hour;
						a->startmin = tm->tm_min;
					}
					a->showstart = TRUE;
					set_time = TRUE;
				}

				nwhat_lines = 0;

			} else if (strcasecmp(temp,  "end:" ) == 0 ||
				   strcasecmp(temp,  "until:" ) == 0 ||
				   strcasecmp(temp,  "stop:" ) == 0 ||
				   strcasecmp(temp,  "to:" ) == 0) {

				if (!set_date)
					goto ERROR_EXIT;

				temp = (char *)strtok(NULL, "\n");
				if (temp != NULL && !blank_buf(temp)) {
					cm_strcpy(tmp_buf, a->datestr);
					cm_strcat(tmp_buf, " ");
					cm_strcat(tmp_buf, temp);
					enddate = cm_getdate(tmp_buf, NULL); 
					if (enddate < 0)
						goto ERROR_EXIT;
					else {
						tm = localtime(&enddate);
						a->endhr = tm->tm_hour;
						a->endmin = tm->tm_min;
					}
					a->showstop = TRUE;
				}
				nwhat_lines = 0;

			} else  if (strcasecmp(temp,  "what:" )==0) {
				if ((temp = (char *)strtok(NULL,"\n")) != NULL)
					while (*temp == ' ' || *temp == '\t')
						temp++;
	
				if (a->what1 != NULL) {
					free(a->what1);
					a->what1 = NULL;
				}

				if (temp && !blank_buf(temp)) {
					what_len = cm_strlen(temp) + 1;
					a->what1 = (char *) calloc(1, what_len);
					cm_strcpy (a->what1, temp);
					nwhat_lines = 1;
				}
				findwhat = TRUE;

			} else if (strcasecmp(temp, "repeat:") == 0) {
				if ((temp=(char *)strtok(NULL,"\n")) != NULL) {
					while (*temp == ' ' || *temp == '\t')
						temp++;
					ptr = &temp[cm_strlen(temp)-1];
					while (*ptr &&
					       ptr > temp && isspace(*ptr)) {
						*ptr = '\0';
						ptr--;
					}

					a->repeat = pstr_to_unitsCloc(temp,
							&(a->nth));
					if (a->repeat == -1) {
						ptr = strrchr(temp, ',');
						if (ptr)
							*ptr = '\0';
						else
							goto ERROR_EXIT;
						a->repeat = pstr_to_unitsCloc(
							temp, &(a->nth));
						if (a->repeat == -1)
							goto ERROR_EXIT;
					}

					if (a->repeat == nthWeekday && ptr++) {
						while (*ptr==' ' || *ptr=='\t')
							ptr++;
						if (*ptr &&
						    !strncasecmp(ptr,"last",4))
							a->nth = -1;
						else
							a->nth = atoi(ptr);
					}

					if (set_ntimes == FALSE)
						a->ntimes = atoi(
							repeatval[a->repeat]);
				}
				nwhat_lines = 0;

			} else if (strcasecmp(temp, "for:") == 0) {
				if ((temp=(char *)strtok(NULL,"\n")) != NULL) {
					while (*temp == ' ' || *temp == '\t')
						temp++;

					if (strcasecmp(temp, "forever"))
						a->ntimes = atoi(temp);
					else
						a->ntimes = -1;
					set_ntimes = TRUE;
				}
				nwhat_lines = 0;

			} else if (findwhat && nwhat_lines < 4) {
				what_len = cm_strlen(temp) + 1;
				temp2 = (char *)strtok(NULL,"\n");
				if (temp2 != NULL)
					what_len += cm_strlen(temp2) + 2;
				else {
					ptr = &temp[cm_strlen(temp)-1];
					while (*ptr && ptr>temp && *ptr=='\n') {
						*ptr = '\0';
						ptr--;
					}
				}

				ptr = (char *)calloc(1, what_len);
				cm_strcat (ptr, temp);
				if (temp2 != NULL) {
					cm_strcat (ptr, " ");
					cm_strcat (ptr, temp2);
				}

				ptrptr = &a->what1;
				ptrptr += nwhat_lines;
				*ptrptr = ptr;
				nwhat_lines++;
			}
		}

		fclose(f);
	}

done:
	if (!set_date)
		goto ERROR_EXIT;
	
	if ((a->repeat && a->ntimes == 0) || (a->repeat == 0 && a->ntimes != 0))
		goto ERROR_EXIT;

	xv_set(ip->window, FRAME_BUSY, FALSE, 0);
	show_appt(ip, a);
	return(0);

ERROR_EXIT:
	if (f != NULL) fclose(f);
	destroy_appt(a);

	if (fromdnd) {
		notice_prompt(ip->window, (Event *)NULL,
			NOTICE_MESSAGE_STRINGS,
			EGET("Unable to display appointment:"),
			EGET("Invalid appointment format."),
			NULL,
			NOTICE_BUTTON_YES, LGET("Continue"),
			NULL);
	} else {
		notice_prompt(ip->window, (Event *)NULL,
			NOTICE_MESSAGE_STRINGS,
			EGET("Unable to display appointment:"),
			EGET("Invalid appointment format."),
			EGET("Will display default values."),
			NULL,
			NOTICE_BUTTON_YES, LGET("Continue"),
			NULL);
		set_default_values(ip);
	}

	xv_set(ip->window, FRAME_BUSY, FALSE, 0);
	return(-1);
}

/*
 * load appointment data
 */
extern int
load_data(char *data, int size, int flag)
{
	char filename[15];
	FILE *fd;
	int result;

	cm_strcpy(filename, "/tmp/aeXXXXXX");
	mktemp(filename);

	fd = fopen(filename, "w");
	if (fd) {
		fwrite(data, 1, size, fd);
		fclose(fd);

		result = load_proc(Ae_window, filename, flag);
		unlink(filename);
		return(result);
	}
	return(-1);
}

static void
attach_help_keywords()
{
	xv_set(Ae_window->controls, XV_HELP_DATA, "cm:MaePanel", NULL);
	xv_set(Ae_window->droptarget, XV_HELP_DATA, "cm:MaeDnDTarget", NULL);
	xv_set(Ae_window->datelabel, XV_HELP_DATA, "cm:MaeDateField", NULL);
	xv_set(Ae_window->date_textfield, XV_HELP_DATA, "cm:MaeDateField", NULL);
	xv_set(Ae_window->start_ampm, XV_HELP_DATA, "cm:ApptTimeUnit", NULL);
	xv_set(Ae_window->start_button, XV_HELP_DATA, "cm:MaeApptTime", NULL);
	xv_set(Ae_window->start_textfield, XV_HELP_DATA, "cm:MaeApptTime", NULL);
	xv_set(Ae_window->stop_ampm, XV_HELP_DATA, "cm:ApptTimeUnit", NULL);
	xv_set(Ae_window->stop_button, XV_HELP_DATA, "cm:MaeApptDuration", NULL);
	xv_set(Ae_window->stop_textfield, XV_HELP_DATA, "cm:MaeApptDuration", NULL);
	xv_set(Ae_window->what_textfield1, XV_HELP_DATA, "cm:MaeWhatField", NULL);
	xv_set(Ae_window->what_textfield2, XV_HELP_DATA, "cm:MaeWhatField", NULL);
	xv_set(Ae_window->what_textfield3, XV_HELP_DATA, "cm:MaeWhatField", NULL);
	xv_set(Ae_window->what_textfield4, XV_HELP_DATA, "cm:MaeWhatField", NULL);
	xv_set(Ae_window->repeat_button, XV_HELP_DATA, "cm:RepeatStyle", NULL);
	xv_set(Ae_window->repeat_message, XV_HELP_DATA, "cm:RepeatUnit", NULL);
	xv_set(Ae_window->for_textfield, XV_HELP_DATA, "cm:RepeatTimes", NULL);
	xv_set(Ae_window->days_message, XV_HELP_DATA, "cm:RepeatTimes", NULL);
	xv_set(Ae_window->for_button, XV_HELP_DATA, "cm:RepeatTimes", NULL);
	xv_set(Ae_window->attach_button, XV_HELP_DATA, "cm:MaeAttachButton", NULL);
	xv_set(Ae_window->reset_button, XV_HELP_DATA, "cm:EdClearButton", NULL);

	xv_set(Ae_repeat_win->repeat_control, XV_HELP_DATA, "cm:RepeatHelp", NULL);
	xv_set(Ae_repeat_win->repeatunit, XV_HELP_DATA, "cm:Repeatinterval", NULL);
	xv_set(Ae_repeat_win->repeat_menu, XV_HELP_DATA, "cm:RepeatUnitMenu", NULL);
	xv_set(Ae_repeat_win->repeatunitmessage, XV_HELP_DATA, "cm:RepeatUnitMenu", NULL);
	xv_set(Ae_repeat_win->apply_button, XV_HELP_DATA, "cm:RepeatApply", NULL);
}

/*
 * Instance XV_KEY_DATA key.  An instance is a set of related
 * user interface objects.  A pointer to an object's instance
 * is stored under this key in every object.  This must be a
 * global variable.
 */
Attr_attribute	INSTANCE;

static int
init_ae(int argc, char **argv)
{
	struct stat info;
	int	c;
	extern	char	*optarg;
	extern	int	optind;
	int	cmd, loadfile = FALSE;
	char	*defaultdatemsg;
	char	*datemsg;
	char	bind_home[MAXPATHLEN];
	Rc_value *rcvalue;
	char	**av = argv;
	Xv_Font	newfont;

#ifdef AE_DEBUG
	debug = 1;
#endif

	/* check for -tooltalk */
	while (*av) {
		if (strcmp(*av, "-tooltalk") == 0)
			tt_flag = TRUE;
		av++;
	}

	/*
	 * Iinitialize tooltalk. 
	 * Even if it is not invoked by tooltalk, ae accepts
	 * file_modify message to update it's display when
	 * user changes cm properties.
	 */
	if (dstt_check_startup(get_version, &argc, &argv)) {
		if (debug)
			fprintf(stderr, "Could not initialize tooltalk\n");
		return(1);
	}

	ds_expand_pathname("$OPENWINHOME/lib/locale", bind_home);
	bindtextdomain(MSGFILE_ERROR, bind_home);
	bindtextdomain(MSGFILE_LABEL, bind_home);
	bindtextdomain(MSGFILE_MESSAGE, bind_home);

	/*
	 * Initialize XView.
	 */
	xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv,
		XV_USE_LOCALE, TRUE,
		NULL);
	INSTANCE = xv_unique_key();

	/* Must set XV_USE_LOCALE to TRUE before calling gettext */
	defaultdatemsg = MGET("Month / Day / Year");

	if (!tt_flag) {
		cmd = TRUE;
		while((c = getopt(argc, argv, "?d")) != -1) {
			switch (c) {
			case 'd':
				debug = 1;
				/*
				fprintf(stderr,
					"c = %c optarg =  '%s' optind = %d\n",
					c, optarg, optind);
				*/
				break;
			case '?':
			default:
				fprintf(stderr, "Usage: %s [-d] filename\n",
					argv[0]);
				break;
			}
		}

		if(cmd)
		{
			if(argv[optind] != NULL) {
				file = argv[optind];
			} else {
				fprintf(stderr, "Usage: %s [-d] filename\n",
					argv[0]);
				exit(1);
			}
		}

		if(stat(file, &info) == -1) {
			int	n = 0666;

			if (errno == ENOENT) {
				n = creat(file, n);
				if(n == -1) {
					perror(file);
					exit(1);
				} else
					close(n);
			} else {
				perror(file);
				exit(1);
			}
		} else if (info.st_size > 0)
			loadfile = TRUE;

	} /* if !tt_flag */

	/* strings used for the repeat and for fields */
	init_periodstr();

	/*
	 * Initialize user interface components.
	 * Do NOT edit the object initializations by hand.
	 */
	Ae_window = ae_window_objects_initialize(NULL, NULL);
	Ae_repeat_win = ae_repeat_win_objects_initialize(NULL,
				Ae_window->window);

	/* attach help text keywords */
	attach_help_keywords();

	if (get_rc_value() == -1) {
		fprintf(stderr, "ae: cannot allocate memory\n");
		exit(1);
	} else
		rcvalue = (Rc_value *)xv_get(Ae_window->window, WIN_CLIENT_DATA);

	ae_make_menus();

	/* set notify level */
	xv_set(Ae_window->date_textfield, PANEL_NOTIFY_LEVEL, PANEL_ALL, NULL);
	xv_set(Ae_window->start_textfield, PANEL_NOTIFY_LEVEL, PANEL_ALL, NULL);
	xv_set(Ae_window->what_textfield1, PANEL_NOTIFY_LEVEL, PANEL_ALL, NULL);
	xv_set(Ae_window->what_textfield2, PANEL_NOTIFY_LEVEL, PANEL_ALL, NULL);
	xv_set(Ae_window->what_textfield3, PANEL_NOTIFY_LEVEL, PANEL_ALL, NULL);
	xv_set(Ae_window->what_textfield4, PANEL_NOTIFY_LEVEL, PANEL_ALL, NULL);

	/* set date message, use the same font as cm */
	newfont = (Xv_Font) xv_find(Ae_window->controls, FONT,
#ifdef OW_I18N
		FONT_FAMILY,	FONT_FAMILY_SANS_SERIF,
		FONT_SCALE,	WIN_SCALE_SMALL,
#else
		FONT_FAMILY,	FONT_FAMILY_LUCIDA,
		FONT_SIZE,	8,
#endif
		NULL);

	if (newfont == NULL)
		newfont = (Xv_font)xv_get(Ae_window->controls, XV_FONT);

	if ((datemsg = get_datemsg(rcvalue->order, rcvalue->sep)) == NULL)
		xv_set(Ae_window->date_message, PANEL_LABEL_FONT, newfont,
			PANEL_LABEL_STRING, defaultdatemsg, NULL);
	else {
		xv_set(Ae_window->date_message, PANEL_LABEL_FONT, newfont,
			PANEL_LABEL_STRING, datemsg, NULL);
		free(datemsg);
	}

	/* drag and drop */
	gdd_init_dragdrop(Ae_window->window);

	if (loadfile)
		load_proc(Ae_window, file, FALSE);
	else
		set_default_values(Ae_window);

	/* complete tooltalk initialization */
	ae_dstt_start(Ae_window->window, tt_flag);

	/* initialize data_changed flag */
	data_changed = FALSE;

	return(0);
}

main(int argc, char **argv)
{
	if (init_ae(argc, argv))
		exit(1);

	if (tt_flag)
		xv_set(Ae_window->window, XV_X, -400, XV_Y, -400, NULL);

	/*
	 * Turn control over to XView.
	 */
	xv_main_loop(Ae_window->window);

	exit(0);
}

