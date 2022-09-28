#ifndef lint
static  char sccsid[] = "@(#)cm_i18n.c 3.5 94/05/13 Copyr 1991 Sun Microsystems, Inc.";
#endif
/* cm_i18n.c */

#include <stdio.h>
#include <string.h>
#include <floatingpoint.h>
#include <locale.h>
#include <sys/param.h>       /* for MAXPATHLEN */
#include <xview/xview.h>
#include <xview/generic.h>
#include "util.h"
#include "timeops.h"
#include "cm_i18n.h"
#include "gettext.h"

char *fontset1[2]; 
char *fontset2[2];
int use_default_fonts = FALSE;
int use_octal = FALSE;


/* Routines to access cm font file */

int cm_get_fonts(locale) 
char *locale;
{
	FILE *fp;
	char fontfile[MAXPATHLEN];
	char line[MAX_LINE_LEN];
	int found_locale = 0;

	ds_expand_pathname("$OPENWINHOME/lib/locale", fontfile);
	cm_strcat(fontfile, "/");
	cm_strcat(fontfile, locale);
	cm_strcat(fontfile, "/");
	cm_strcat(fontfile, "app-defaults/CM_FONTFILE");
	if ( (fp = fopen(fontfile, "r")) == NULL ) {
		if ( strcmp(locale, "C") ) {
			fprintf(stderr, EGET("cm_get_fonts():  Cannot open %s\n"), fontfile);
		}
		return OPEN_FAIL;
		/* NOTE:  IF OPEN_FAIL and in C locale, then use the hard coded
	      *        fonts, else user must have defined his/her own C fonts.
		 */
	}
	while ( fgets(line, MAX_LINE_LEN, fp) ) {
		if ( !is_comment(line) ) {
			if ( match_locale(locale, line) ) {
				found_locale = 1;
				switch (extract_fontsets(fp, line)) {
					case 0:
						fprintf(stderr, EGET("cm_get_fonts():  Cannot extract font set names\n"));
						fclose(fp);
						return EXTRACT_FAIL;
					case 1:
						fclose(fp);
						return NO_I18N_HEADER;
				}
				break;   
			}
		}
	}
	if ( !found_locale ) {
		fprintf(stderr, EGET("cm_get_fonts():  Cannot find locale %s entry in CM_FONTFILE\n"), locale);
		fclose(fp);
		return NO_LOCALE_ENTRY;
	}
	fclose(fp);
	return OKAY;
}


int is_comment(line)
char line[MAX_LINE_LEN];
{
	char ch[2];

	strncpy(ch, line, 1);
	if ( strcmp((char *)ch, COMMENT_SYMBOL) == 0 ) {
		return 1;	
	} else {
		return 0;	
	}
}


int match_locale(locale, line)
char *locale;
char line[MAX_LINE_LEN];
{
	char loc[MAX_LINE_LEN];	

	if ( !isalpha(line[0]) ) {
		return 0;
	}
	(void) sscanf(line, "%s", loc);
	if ( strcmp(loc, locale) == 0 ) {
		return 1;
	} else {
		return 0;
	}
}


int extract_fontsets(fp, line)
FILE *fp;
char line[MAX_LINE_LEN];
{
	char arg1[MAX_FONT_LEN], arg2[MAX_FONT_LEN], arg3[MAX_FONT_LEN];
	int num_scanned;

	if ( sscanf(line, "%s%s%s", arg1, arg2, arg3) != 3 ) {
		return 0;
	}
	fontset1[0] = (char *)malloc(CHAR_SIZE * cm_strlen(arg2));
	fontset2[0] = (char *)malloc(CHAR_SIZE * cm_strlen(arg3));
	cm_strcpy(fontset1[0], arg2);
	cm_strcpy(fontset2[0], arg3);
	if ( strcmp(arg1, "C") == 0 ) {
		return 1;
	}
	if ( !fgets(line, MAX_LINE_LEN, fp) ) {
		return 1;
	}
	if ( (num_scanned = sscanf(line, "%s%s%s", arg1, arg2, arg3)) == 3 ) {
		return 1;
	} else if ( num_scanned == 2 ) {
		fontset1[1] = (char *)malloc(CHAR_SIZE * cm_strlen(arg1));
		fontset2[1] = (char *)malloc(CHAR_SIZE * cm_strlen(arg2));
		cm_strcpy(fontset1[1], arg1);
		cm_strcpy(fontset2[1], arg2);
		use_octal = TRUE;
		return 2;
	} else {
		return 0;
	}
}


/* This header is needed in the postscript file for i18n printing. 
 */
void ps_i18n_header(fp, frame) 
FILE *fp;
Frame frame;
{
	char *display_locale;
	int c_locale = 0;

	display_locale = (char *)xv_get(frame, XV_LC_DISPLAY_LANG);
	c_locale = strcmp(display_locale, "C");
	switch (cm_get_fonts(display_locale)) {
		case OPEN_FAIL:
			if ( c_locale == 0 ) {
				use_default_fonts = TRUE;
			} 
			return;
		case EXTRACT_FAIL: 
			fprintf(stderr, EGET("Cannot print...something wrong with CM_FONTFILE\n"));
			return;
		case NO_I18N_HEADER:
		case NO_LOCALE_ENTRY:
			return;
	}

	fprintf(fp, "\n");
	fprintf(fp, "12 dict begin\n");
	fprintf(fp, "\t/FontName /CMFontSetOne def\n");
	fprintf(fp, "\t/FontType 0 def\n");
	fprintf(fp, "\t/WMode 0 def\n");
	fprintf(fp, "\t/FMapType 3 def\n");
	fprintf(fp, "\t/FontMatrix matrix def\n");
	fprintf(fp, "\t/Encoding [0 1] def\n");
	fprintf(fp, "\t/FDepVector [\n");
	fprintf(fp, "\t\t/%s findfont \n", fontset1[0]);
	fprintf(fp, "\t\t/%s findfont \n", fontset1[1]);
	fprintf(fp, "\t] def\n");
	fprintf(fp, "\tFontName currentdict\n");
	fprintf(fp, "end\n");
	fprintf(fp, "definefont pop\n");
	fprintf(fp, "\n");
	fprintf(fp, "12 dict begin\n");
	fprintf(fp, "\t/FontName /CMFontSetTwo def\n");
	fprintf(fp, "\t/FontType 0 def\n");
	fprintf(fp, "\t/WMode 0 def\n");
	fprintf(fp, "\t/FMapType 3 def\n");
	fprintf(fp, "\t/FontMatrix matrix def\n");
	fprintf(fp, "\t/Encoding [0 1] def\n");
	fprintf(fp, "\t/FDepVector [\n");
	fprintf(fp, "\t\t/%s findfont \n", fontset2[0]);
	fprintf(fp, "\t\t/%s findfont \n", fontset2[1]);
	fprintf(fp, "\t] def\n");
	fprintf(fp, "\tFontName currentdict\n");
	fprintf(fp, "end\n");
	fprintf(fp, "definefont pop\n");
	fprintf(fp, "\n");
}


/* The following routine is specific to using FMapType 3 composite fonts
 * in postscript.  Kanji, Asian specific?
 */
char *euc_to_octal(srcStr)
char *srcStr;
{
	int inKanji = FALSE;
	char buf[64];
	static char dstStr[512];
	int i;
	int len = cm_strlen(srcStr);

#ifdef SVR4
	memset(dstStr, 0, sizeof(dstStr));
#else
	bzero(dstStr, sizeof(dstStr));
#endif "SVR4"
	for (i = 0; i < len; i++) {
		if (inKanji) {
			if (!isEUC(srcStr[i])) {
				inKanji = FALSE;
				/* NOT NEEDED FOR FMapType 4 (or 5)
				cm_strcat(dstStr, "\\377\\000");
				*/
			}
		}
		else {
			if (isEUC(srcStr[i])) {
				inKanji = TRUE;
				/* NOT NEEDED FOR FMapType 4 (or 5)
				cm_strcat(dstStr, "\\377\\001");
				*/
			}
		}
		if (inKanji) {
			sprintf(buf, "\\%3.3o\\%3.3o", srcStr[i] & 0xff, srcStr[i+1] & 0xff);
			i++;
		}
		else {
			sprintf(buf, "%c", srcStr[i]);
		}
		cm_strcat(dstStr, buf);
	}
	return dstStr;
}

#ifdef SVR4

#include <time.h>
#include <fcntl.h>

char *
strptime(date, fmt, tm)
    char *date;
    char *fmt;
    struct tm *tm;
{
    char *filename;
    int fd;
    char *buf;

    filename = tmpnam(NULL);
    fd = creat(filename, 0666); /* RW */
    write(fd, fmt, cm_strlen(fmt));

    buf = cm_strcat(filename,"\"; export DATEMSK \n");
    buf = cm_strcat("set DATEMSK=\"", buf);
    system(buf); 

    tm = getdate(date);
    if (tm == NULL) printf("cm_strptime: tm is NULL \n");
    return(date);
}
#endif "SVR4"

#if 0   /* NOT needed if new (post-beta) date format is used */
/* Pre-process the date entered (by the user in the appt editor date field)
 * before it goes into the parser to get tick.
 *
 * Arguments:  date = international date string
 *             frame = frame
 *             format = style of date format
 *                      0 = default (XXX XX, XXXX);
 *                      2 = format_date2 style; 3 = format_date3 style.
 * Returns a date in English format.
 *
char *cm_get_i18n_date(frame, date, format)
 */
char *cm_get_i18n_date(frame, date)
Frame frame;
char *date;
{
	struct tm tm;
	char *locale;
	char the_month[4];
	static char the_date[13];   /* English date format: XXX XX, XXXX */

#ifdef SVR4
	memset(the_date, 0, sizeof(the_date));
#else
	bzero(the_date, sizeof(the_date));
#endif "SVR4"
	locale = (char *)xv_get(frame, XV_LC_TIME_FORMAT);
	if ( strcmp(locale, "C") == 0 ) {
		cm_strcpy(the_date, date);
	} else {
		(void)strptime(date, "%x", &tm);
		switch ((tm.tm_mon) + 1) {
			case 1: 
				cm_strcpy(the_month, "Jan");
				break;
			case 2:
				cm_strcpy(the_month, "Feb");
				break;
			case 3:
				cm_strcpy(the_month, "Mar");
				break;
			case 4:
				cm_strcpy(the_month, "Apr");
				break;
			case 5:
				cm_strcpy(the_month, "May");
				break;
			case 6:
				cm_strcpy(the_month, "Jun");
				break;
			case 7:
				cm_strcpy(the_month, "Jul");
				break;
			case 8:
				cm_strcpy(the_month, "Aug");
				break;
			case 9:
				cm_strcpy(the_month, "Sep");
				break;
			case 10:
				cm_strcpy(the_month, "Oct");
				break;
			case 11:
				cm_strcpy(the_month, "Nov");
				break;
			case 12:
				cm_strcpy(the_month, "Dec");
				break;
			default:
				fprintf(stderr, EGET("cm_get_i18n_date(): no such month\n"));
				exit(-30);
		}
	}
	/*
char yr[4];
	switch (format) {
		case 2:
			yr[0] = '\0';
			sprintf(yr, "%d", tm.tm_year);
			sprintf(the_date, "%d/%.2d/%s", tm.tm_mon+1, tm.tm_mday, yr);
			break;
		case 3:
			sprintf(the_date, "%d/%.2d", tm.tm_mon+1, tm.tm_mday);
			break;
		case 0:
		default:
			sprintf(the_date, "%s %d, %d", the_month, tm.tm_mday, tm.tm_year);
			break;
	}
	*/
	return the_date;
}
#endif  /* 0 */


/* Convert tick to date format.
 */
void convert_date(tick, date, frame)
Tick tick;
char *date;
Frame frame;
{
	char *locale;
	char the_day[10];
	char the_month[4];
	struct tm *tm;
	int dd;
	int yy;

	locale = (char *)xv_get(frame, XV_LC_TIME_FORMAT);
	if ( strcmp(locale, "C") == 0 ) {
		return;
	} else {
		tm = localtime(&tick);
		yy = tm->tm_year;
		dd = tm->tm_mday;
		switch (tm->tm_wday) {
			case 1:	
				cm_strcpy(the_day, "Monday");
				break;
			case 2:
				cm_strcpy(the_day, "Tuesday");
				break;
			case 3:
				cm_strcpy(the_day, "Wednesday");
				break;
			case 4:
				cm_strcpy(the_day, "Thursday");
				break;
			case 5:
				cm_strcpy(the_day, "Friday");
				break;
			case 6:
				cm_strcpy(the_day, "Saturday");
				break;
			case 0:
				cm_strcpy(the_day, "Sunday");
				break;
			default:
				fprintf(stderr, "convert_date(): no such week day, %d\n", tm->tm_wday);
				exit(-10);
		}
		switch ((tm->tm_mon) + 1) {
			case 1: 
				cm_strcpy(the_month, "Jan");
				break;
			case 2:
				cm_strcpy(the_month, "Feb");
				break;
			case 3:
				cm_strcpy(the_month, "Mar");
				break;
			case 4:
				cm_strcpy(the_month, "Apr");
				break;
			case 5:
				cm_strcpy(the_month, "May");
				break;
			case 6:
				cm_strcpy(the_month, "Jun");
				break;
			case 7:
				cm_strcpy(the_month, "Jul");
				break;
			case 8:
				cm_strcpy(the_month, "Aug");
				break;
			case 9:
				cm_strcpy(the_month, "Sep");
				break;
			case 10:
				cm_strcpy(the_month, "Oct");
				break;
			case 11:
				cm_strcpy(the_month, "Nov");
				break;
			case 12:
				cm_strcpy(the_month, "Dec");
				break;
			default:
				fprintf(stderr, "convert_date: no such month, %d\n", tm->tm_mon);
				exit(-30);
		}
		sprintf(date, "%s %s %d, %2d", the_day, the_month, dd, yy);
	}
}

/* This routine should be in libdeskset.
 * This routine uses fconvert() to avoid locale conversion.
 */
/* 310 characters are the minimum needed to accommodate any double-precision
 * value + 1 null terminator.
 */
#define DBL_SIZE  311
/*
 *  Returns a null terminated formatted string.
 *  If error is encountered, such as malloc() failed, then return NULL.
 *  The caller of this function should beware that the return value is
 *  a static buffer declared within this function and the value of it may
 *  change.
 */
char *
cm_printf(value, decimal_pt)
double value;
int decimal_pt;
{
	int sign = 0;
	int deci_pt = 0;
	int buf_cnt = 0;
	int formatted_cnt = 0;
	int buf_len = 0;
	char *buf = NULL;
	static char *formatted = NULL;

	if ( formatted != NULL ) {
		free(formatted);
		formatted = NULL;
	}
	if ( (value == (double)0) && (decimal_pt == 0) ) {
		formatted = (char *)strdup("0");
		return formatted;
	}
	if ( (buf = (char *)malloc(DBL_SIZE + decimal_pt)) == NULL ) {
		return (char *)NULL;
	}
	if ( (formatted = (char *)calloc(1, DBL_SIZE + decimal_pt)) == NULL ) {
		return (char *)NULL;
	}
	fconvert(value, decimal_pt, &deci_pt, &sign, buf);
	if ( sign ) {
		strcpy(formatted, "-");
	}
	buf_len = deci_pt + decimal_pt;
	if ( deci_pt ) {
		strncat(formatted, buf, deci_pt);
	} else {    /* zero */
		strcat(formatted, "0");	
	}
	if ( deci_pt == buf_len ) {
		strcat(formatted, "\0");
		return formatted;
	}
	strcat(formatted, ".");
	for ( formatted_cnt = strlen(formatted), buf_cnt = deci_pt;  buf_cnt < buf_len;  buf_cnt++, formatted_cnt++ ) {
		formatted[formatted_cnt] = buf[buf_cnt];
	}
	formatted[formatted_cnt] = '\0';
	free(buf);
	return formatted;	
}
