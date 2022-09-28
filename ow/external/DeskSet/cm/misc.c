#ifndef lint
static  char sccsid[] = "@(#)misc.c 1.20 97/05/15 Copyr 1991 Sun Microsystems, Inc.";
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
#include <time.h>
#include <limits.h>
#include <sys/stat.h>
#include "appt.h"
#include "util.h"
#include "datefield.h"
#include "gettext.h"
#include "misc.h"

char *separator[] = {
	" ", "/", ".", "-"
};

/* put these here so that they are defined in libcm.a */
char *MSGFILE_ERROR = "SUNW_DESKSET_CM_ERR";
char *MSGFILE_LABEL = "SUNW_DESKSET_CM_LABEL";
char *MSGFILE_MESSAGE = "SUNW_DESKSET_CM_MSG";

char *periodstr_Cloc[15];	/* repeating event strings in C locale */
char *periodstr[15];		/* repeating event strings in local locale */
char *repeatstr[15];		/* scope of repeating events */
char *repeatval[15];		/* default number of times */ 

extern void
init_periodstr()
{
	periodstr_Cloc[0] = "One Time";
	periodstr_Cloc[1] = "Daily";
        periodstr_Cloc[2] = "Weekly";
        periodstr_Cloc[3] = "Every Two Weeks";
        periodstr_Cloc[4] = "Monthly By Date";
        periodstr_Cloc[5] = "Yearly";
        periodstr_Cloc[6] = "Monthly By Weekday";
	periodstr_Cloc[7] = "days";
        periodstr_Cloc[8] = "weeks";
        periodstr_Cloc[9] = "months";
        periodstr_Cloc[10] = "other";
        periodstr_Cloc[11] = "Monday thru Friday";
        periodstr_Cloc[12] = "Mon, Wed, Fri";
        periodstr_Cloc[13] = "Tuesday, Thursday";
        periodstr_Cloc[14] = "Weekday Combo";

	periodstr[0] = MGET("One Time");
        periodstr[1] = MGET("Daily");
        periodstr[2] = MGET("Weekly");
        periodstr[3] = MGET("Every Two Weeks");
        periodstr[4] = MGET("Monthly By Date");
        periodstr[5] = MGET("Yearly");
        periodstr[6] = MGET("Monthly By Weekday");
        /* every nth days, weeks, months */
        periodstr[7] = MGET("days");
        periodstr[8] = MGET("weeks");
        periodstr[9] = MGET("months");
        periodstr[10] = MGET("other");
        periodstr[11] = MGET("Monday thru Friday");
        periodstr[12] = MGET("Mon, Wed, Fri");
        periodstr[13] = MGET("Tuesday, Thursday");
        periodstr[14] = MGET("Weekday Combo");

	repeatstr[0] = "";
        repeatstr[1] = MGET("days");
        repeatstr[2] = MGET("weeks");
        repeatstr[3] = MGET("biweeks");
        repeatstr[4] = MGET("months");
        repeatstr[5] = MGET("years");
        repeatstr[6] = MGET("months");
        repeatstr[7] = MGET("days");
        repeatstr[8] = MGET("weeks");
        repeatstr[9] = MGET("months");
        repeatstr[10] = MGET("months");
        repeatstr[11] = MGET("weeks");
        repeatstr[12] = MGET("weeks");
        repeatstr[13] = MGET("weeks");
        repeatstr[14] = MGET("weeks");
 
	repeatval[0] = "";
	repeatval[1] = "365";
	repeatval[2] = "52";
	repeatval[3] = "26";
	repeatval[4] = "12";
	repeatval[5] = "2";
	repeatval[6] = "12";
	repeatval[7] = "365";
	repeatval[8] = "52";
	repeatval[9] = "12";
	repeatval[10] = "12";
	repeatval[11] = "52";
	repeatval[12] = "52";
	repeatval[13] = "52";
	repeatval[14] = "52";
}

/*
 * Return a date label according to order and sep
 */
extern char *
get_datemsg(Ordering_Type order, Separator_Type sep)
{
	char buf[256];

	/* set date message */
	switch (order) {
	case Order_DMY:
		sprintf(buf, "%s %s %s %s %s", LGET("Day"), separator[sep],
			LGET("Month"), separator[sep], LGET("Year"));
		break;
	case Order_YMD:
		sprintf(buf, "%s %s %s %s %s", LGET("Year"), separator[sep],
			LGET("Month"), separator[sep], LGET("Day"));
		break;
	case Order_MDY:
	default:
		sprintf(buf, "%s %s %s %s %s", LGET("Month"), separator[sep],
			LGET("Day"), separator[sep], LGET("Year"));
		break;
	}
	return(cm_strdup(buf));
}

/*
 * format the date according to display property and write it into buffer
 */
extern void
format_tick(long tick, Ordering_Type order, Separator_Type sep, char *buff)
{
	struct tm *tm;

	buff[0] = NULL;
	tm = localtime(&tick);

	if (tm->tm_year > 99) tm->tm_year -= 100;
	switch (order) {
	case Order_DMY:
		sprintf(buff, "%d%s%d%s%02d", tm->tm_mday, separator[sep],
			tm->tm_mon+1, separator[sep], tm->tm_year);
		break;
	case Order_YMD:
		sprintf(buff, "%02d%s%d%s%d", tm->tm_year, separator[sep],
			tm->tm_mon+1, separator[sep], tm->tm_mday);
		break;
	case Order_MDY:
	default:
		sprintf(buff, "%d%s%d%s%02d", tm->tm_mon+1, separator[sep],
			tm->tm_mday, separator[sep], tm->tm_year);
		break;
	}
}

/*
 * Parse the date string and get the month, day, and year
 */
extern int
parse_date(Ordering_Type order,
	Separator_Type sep,
	char *datestr,
	char *m,
	char *d,
	char *y)
{
        char *first, *second, *third;
        char *tmp_date;
 
	m[0] = NULL;
	d[0] = NULL;
	y[0] = NULL;

	if (datestr == NULL)
		return 0;

        tmp_date = cm_strdup(datestr);
        first = strtok(tmp_date, separator[sep]);
		/* Check to see if the date entered has legit separator */
		if ( strcoll(first, datestr) == 0 ) {    /* separator not found */
			free(tmp_date);
			return 0;
		}
        second = strtok(NULL, separator[sep]);
        third = strtok(NULL, separator[sep]);

	switch (order) {
	case Order_DMY:
		if (second)
                	cm_strcpy(m, second);
		if (first)
                	cm_strcpy(d, first);
		if (third)
                	cm_strcpy(y, third);
		break;
	case Order_YMD:
		if (second)
                	cm_strcpy(m, second);
		if (third)
                	cm_strcpy(d, third);
		if (first)
                	cm_strcpy(y, first);
		break;
	case Order_MDY:
	default:
		if (first)
                	cm_strcpy(m, first);
		if (second)
                	cm_strcpy(d, second);
		if (third)
                	cm_strcpy(y, third);
		break;
	}
        free(tmp_date);
		return 1;
}

/*
 * Reformat the date string into m/d/y format and write it into the buffer
 */
extern int
datestr2mdy(char *datestr, Ordering_Type order, Separator_Type sep, char *buf)
{
	char m[3], d[3], y[5];

	buf[0] = NULL;
	if (datestr == NULL)
		return 0;

	if (order == Order_MDY && sep == Separator_Slash) { 
		if (strstr(datestr, LGET("/")) != NULL) {
			cm_strcpy(buf, datestr);
			return 1;
		}
		return 0;
	}
	else { 
		if (parse_date(order, sep, datestr, m, d, y)) {
			sprintf(buf, "%s/%s/%02s", m, d, y);
			return 1;
		}
		return 0;
	
	}
}

/*
 * These routines are for applications that want to just
 * get the props without use of a window sytem/ds_laod_resources().
 * It reads the .desksetdefaults file and checks for an
 * "Upgraded" prop. If there is one it reads the props,
 * otherwise it gets the props from .cm.rc.
 */ 

#define COMMENT         '#'
#define CONTINUATION    '\\'

extern void
cm_free_resources(list)
        Pentry    *list;
{
	Pentry *ptr, *n_ptr;

	ptr = list;
	while (ptr != NULL) {
		n_ptr = ptr->next;
		free(ptr->property_name);
		free(ptr->property_value);
		free(ptr);
		ptr = n_ptr;
	}
}

extern Pentry*
cm_get_resources()
{
	FILE	*rc_file;
	char	buffer[BUFSIZ];
	char 	*c_ptr, *d_ptr;
	int	content_gotten;
	Pentry	*new_prop;
	Boolean  upgraded = false;
	Pentry  *pentries = NULL;
	char	*ptr, file_name[256];
	struct  stat    statbuf;
	char     cal_default[2 * BUFSIZ];

	if ((ptr = (char*)getenv("DESKSETDEFAULTS")) == NULL)
		sprintf(file_name, "%s/%s", getenv("HOME"), DS_FILENAME);
	else
		cm_strcpy(file_name, ptr);
	/* if the file exists, check for "Upgraded" property. */
	if (stat(file_name, &statbuf) != -1) {
		rc_file = fopen(file_name, "r");
		if (rc_file != NULL) {
			/* see if they've upgraded yet from .cm.rc */
			while ((int)fgets(buffer, BUFSIZ-1, rc_file)) {
				c_ptr = (char*)strchr(buffer, ':');
				if (c_ptr != NULL && *c_ptr != NULL)
					*c_ptr = NULL;
				if (strcmp(buffer, "deskset.calendar.Upgraded") == 0) {
					rewind(rc_file);
					upgraded = true;
					break;
				}
			}
		}
	}
	/* if they havent upgraded to .desksetdefaults,
		 read the .cm.rc file */
	if (!upgraded) {
		/* .cm.rc. file */
		sprintf(file_name, "%s/%s", getenv("HOME"), RC_FILENAME);
		if (stat(file_name, &statbuf) != -1) {
			rc_file = fopen(file_name, "r");
			if (rc_file == NULL) return NULL;
		}
		else return NULL;
		while ((int)fgets(buffer, BUFSIZ-1, rc_file)) {
                	c_ptr = (char*)strchr(buffer, ':');
                	if (c_ptr) {
                        	*c_ptr = NULL;
                        	new_prop = (Pentry*)ckalloc(sizeof(Pentry));
                        	new_prop->property_name = (char*)cm_strdup(buffer);
                        	new_prop->next = pentries;
                        	pentries = new_prop;
                	}
                	else
                       		continue;
                	d_ptr = cal_default;
                	*d_ptr = NULL;
                	c_ptr++;
                	while (c_ptr != NULL && *c_ptr != NULL
                       		 && (*c_ptr == ' ' || *c_ptr == '\t'))
                        	c_ptr++;
                	if (c_ptr == NULL || *c_ptr == NULL) {
                        	new_prop->property_value = (char*)cm_strdup("");
                        	continue;
			}
        
                	content_gotten = false;
			while (!content_gotten) {
                        	while (*c_ptr && (*c_ptr != COMMENT))
                                	*d_ptr++ = *c_ptr++;
 
	                        if (*c_ptr == COMMENT) {
                                	*d_ptr++ = NULL;
                                	new_prop->property_value = (char*)cm_strdup(cal_default);
                                	content_gotten = true;
                        	}
                        	else if (*(c_ptr - 2) != CONTINUATION) {
                                	*d_ptr++ = NULL;
                                	if (cal_default[cm_strlen(cal_default)
-1] == '\n')
                                       		 cal_default[cm_strlen(cal_default) - 1] = NULL;
					new_prop->property_value = (char*)cm_strdup(cal_default);
					content_gotten = true;
                        	}
                        	else if (!fgets(buffer, BUFSIZ, rc_file)) {
                                	*d_ptr++ = NULL;
                                	new_prop->property_value = (char*)cm_strdup(cal_default);
                                	content_gotten = true;
                        	}
 
                        	d_ptr -= 2;
                        	c_ptr = buffer;
                	}
		}
		fclose(rc_file);
	}
	else {
		/* .desksetdefaults file */
		if (rc_file == NULL) return NULL;
		while ((int)fgets(buffer, BUFSIZ-1, rc_file)) {
                	c_ptr = (char*)strchr(buffer, ':');
                	if (c_ptr == NULL) continue;
			*c_ptr = NULL;
			new_prop = (Pentry*)ckalloc(sizeof(Pentry));
			new_prop->property_name = (char*)cm_strdup(buffer);
			new_prop->next = pentries;
			pentries = new_prop;
                        for (c_ptr++; c_ptr != NULL && *c_ptr != NULL
                                 && (*c_ptr == ' ' || *c_ptr == '\t'); c_ptr++);
                        if (c_ptr == NULL || *c_ptr == NULL) {
                                new_prop->property_value = (char*)cm_strdup("");
                                continue;
                        }
			new_prop->property_value = (char*)cm_strdup(c_ptr);
			/* get rid of \n */
			new_prop->property_value[cm_strlen(new_prop->property_value)-1] = NULL;
		}
		fclose(rc_file);
	}
	return pentries;
}

extern char*
period_to_str(period, nth)
        int period;
	int nth;
{
        static char pstr[80];
        char *every = MGET("Every"); 
 
        if (period == everyNthDay) 
                sprintf(pstr, "%s %d %s", every, nth,
                        periodstr[period]);
        else if (period == everyNthWeek)
                sprintf(pstr, "%s %d %s", every, nth,
                        periodstr[period]);
        else if (period == everyNthMonth)
                sprintf(pstr, "%s %d %s", every, nth,
                        periodstr[period]);
        else
                cm_strcpy(pstr, periodstr[period]);

        return pstr;
}

extern char*
period_to_Clocstr(period, nth)
        int period;
	int nth;
{
        static char pstr[80];
 
        if (period == everyNthDay) 
                sprintf(pstr, "Every %d %s", nth,
                        periodstr_Cloc[period]);
        else if (period == everyNthWeek)
                sprintf(pstr, "Every %d %s", nth,
                        periodstr_Cloc[period]);
        else if (period == everyNthMonth)
                sprintf(pstr, "Every %d %s", nth,
                        periodstr_Cloc[period]);
        else
                cm_strcpy(pstr, periodstr_Cloc[period]);

        return pstr;
}

extern int
pstr_to_units(ps, nth)
        char *ps;
        int *nth;
{
        int per_val = single;
        char *ps2, *ptr, *ptr2, *unit;
 
        if (ps == NULL) return per_val;
        if (strcmp(ps, periodstr[single]) == 0)
                per_val = single;
        else if (strcmp(ps, periodstr[daily]) == 0)
                per_val = daily;
        else if (strcmp(ps, periodstr[weekly]) == 0)
                per_val = weekly;
        else if (strcmp(ps, periodstr[biweekly]) == 0)
                per_val = biweekly;
        else if (strcmp(ps, periodstr[monthly]) == 0)
                per_val = monthly;
        else if (strcmp(ps, periodstr[nthWeekday]) == 0)
                per_val = nthWeekday;
        else if (strcmp(ps, periodstr[yearly]) == 0)
                per_val = yearly;
        else if (strcmp(ps, periodstr[monThruFri]) == 0)
                per_val = monThruFri;
        else if (strcmp(ps, periodstr[monWedFri]) == 0)
                per_val = monWedFri;
        else if (strcmp(ps, periodstr[tueThur]) == 0)
                per_val = tueThur;
        else if (strcmp(ps, periodstr[otherPeriod]) == 0)
                per_val = otherPeriod;
        else if (strcmp(ps, periodstr[daysOfWeek]) == 0)
                per_val = daysOfWeek;
        else {
                unit = strrchr(ps, ' ');
                if (unit == NULL) {
                        per_val = single;
                        return per_val;
                }
		per_val = repeatstr_to_interval(++unit);
		ps2 = cm_strdup(ps);
                ptr = strchr(ps2, ' ');
                ptr++;
                ptr2 = strchr(ptr, ' ');
                ptr2 = NULL;
		*nth = atoi(ptr);
		free(ps2);
        }

        return per_val;
}

extern int
pstr_to_unitsCloc(ps, nth)
        char *ps;
        int *nth;
{
        int per_val = -1;
        char *ps2, *ptr, *ptr2, *unit;
 
        *nth = 0;
        if (ps == NULL) return per_val;
        if (strcasecmp(ps, periodstr_Cloc[single]) == 0)
                per_val = single;
        else if (strcasecmp(ps, periodstr_Cloc[daily]) == 0)
                per_val = daily;
        else if (strcasecmp(ps, periodstr_Cloc[weekly]) == 0)
                per_val = weekly;
        else if ((strcasecmp(ps, periodstr_Cloc[biweekly]) == 0) ||
		 (strcasecmp(ps, "biweekly") == 0)) /* backward compatibility */
                per_val = biweekly;
        else if ((strcasecmp(ps, periodstr_Cloc[monthly]) == 0) ||
		 (strcasecmp(ps, "monthly") == 0)) /* backward compatibility */
                per_val = monthly;
        else if (strcasecmp(ps, periodstr_Cloc[nthWeekday]) == 0)
                per_val = nthWeekday;
        else if (strcasecmp(ps, periodstr_Cloc[yearly]) == 0)
                per_val = yearly;
        else if (strcasecmp(ps, periodstr_Cloc[monThruFri]) == 0)
                per_val = monThruFri;
        else if (strcasecmp(ps, periodstr_Cloc[monWedFri]) == 0)
                per_val = monWedFri;
        else if (strcasecmp(ps, periodstr_Cloc[tueThur]) == 0)
                per_val = tueThur;
        else if (strcasecmp(ps, periodstr_Cloc[otherPeriod]) == 0)
                per_val = otherPeriod;
        else if (strcasecmp(ps, periodstr_Cloc[daysOfWeek]) == 0)
                per_val = daysOfWeek;
        else {
                unit = strrchr(ps, ' ');
                if (unit == NULL) {
                        per_val = single;
                        return per_val;
                }
		per_val = repeatstr_to_Clocinterval(++unit);
		ps2 = cm_strdup(ps);
                ptr = strchr(ps2, ' ');
                ptr++;
                ptr2 = strchr(ptr, ' ');
                ptr2 = NULL;
		*nth = atoi(ptr);
		free(ps2);
        }

        return per_val;
}

extern int
repeatstr_to_Clocinterval(ps)
        char* ps;
{
        int per_val = -1;

	if (strcmp(ps, periodstr_Cloc[everyNthDay]) == 0)
		per_val = everyNthDay;
	else if (strcmp(ps, periodstr_Cloc[everyNthWeek]) == 0)
		per_val = everyNthWeek;
	else if (strcmp(ps, periodstr_Cloc[everyNthMonth]) == 0)
		per_val = everyNthMonth;

        return per_val;
}

extern int
repeatstr_to_interval(ps)
        char* ps;
{
        int per_val = -1;

	if (strcmp(ps, periodstr[everyNthDay]) == 0)
		per_val = everyNthDay;
	else if (strcmp(ps, periodstr[everyNthWeek]) == 0)
		per_val = everyNthWeek;
	else if (strcmp(ps, periodstr[everyNthMonth]) == 0)
		per_val = everyNthMonth;

        return per_val;
}

void
lowercase(s)
char *s;
{
    int i, l;
 
    if (s==NULL) return;
    l = cm_strlen(s);
    for(i=0; i<=l; i++) {
        s[i] = isupper((unsigned char)s[i]) ? tolower((unsigned char)s[i]) : s[i];
    }
}

/* I18N related routine
 * Parameter:  A pointer to a multibyte string.
 * Return Value:  The number of characters in the multibyte string.
 */
extern int
cm_mbstrlen(s)
char *s;
{
	int num_byte = 0, num_char = 0;

	while (*s) {
		if ( (num_byte = mblen(s, MB_LEN_MAX)) <= 0 )
			break;
		num_char++;
		s += num_byte;
	}
	return num_char;
}

/* I18N related routine
 * Parameter:  A pointer to a multibyte string or a pointer to NULL.
 * Return Value:  Returns a pointer to the next multibyte character.
 * Usage:  If the actual argument is non NULL then a pointer to the first multi-
 *         byte character is returned.  If the actual argument is NULL then
 *         a pointer to the next multibyte character is returned.  The parameter
 *         scheme is very much like strtok();
 * CAUTION:  If the calling function uses the return value then it should
 *           make a copy.  The return value is a static buffer that may
 *           be overwritten or freed on subsequent calls to this routine.
 */
extern char *
cm_mbchar(str)
char *str;
{
     static char *string;
     static char *string_head;
     static char *buf;
     int num_byte = 0;
 
     if ( str != NULL ) {
          if ( string != NULL ) {
               free(string_head);
               string_head = NULL;
               string = NULL;
          }
          string = strdup(str);
          string_head = string;
     }
     if ( buf != NULL ) {
          free(buf);
          buf = NULL;
     }
     if ( string == '\0' ) {
          free(string_head);
          string_head = NULL;
     } else {
          num_byte = mblen(string, MB_LEN_MAX);
          buf = (char *)malloc(num_byte+1);
          strncpy(buf, string, num_byte);
          buf[num_byte] = '\0';
          string += num_byte;
     }
 
     return buf;
}
