#ifndef lint
static char sccsid[]="@(#)text.c	1.12 12/15/93 Copyright 1987-1990 Sun Microsystems, Inc." ;
#endif

/*  The DeskSet clock - text initialisation.
 *
 *  Copyright (c) 1988-1990  Sun Microsystems, Inc.
 *  All Rights Reserved.
 *
 *  Sun considers its source code as an unpublished, proprietary
 *  trade secret, and it is available only under strict license
 *  provisions.  This copyright notice is placed here only to protect
 *  Sun in the event the source is deemed a published work.  Dissassembly,
 *  decompilation, or other means of reducing the object code to human
 *  readable form is prohibited by the license agreement under which
 *  this code is provided to the user or company in posesion of this copy.
 *
 *  RESTRICTED RIGHTS LEGEND:  Use, duplication, or disclosure by the
 *  Government is subject to restrictions as set forth in subparagraph
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *  NASA FAR Supplement.
 */

#include <stdio.h>
#include <sys/time.h>
#include "clock.h"

/*  The following are all the static strings used by the clock program.
 *  They are initialised in init_text() to the local language equivalents.
 */

const char *astrs[] = {
  (char *) NULL, (char *) NULL, (char *) NULL
} ;

const char *clk_res[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL
} ;

const char *days[] = {
	(char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, (char *) NULL, (char *) NULL
};

const char *hstr[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL
} ;

const char *hrstrs[] = {
  (char *) NULL, (char *) NULL
} ;
 
const char *mess[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL
} ;

const char *months[] = {
	(char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL
};


const char *month_days[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL
};
 
const char *opts[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL
} ;
 
const char *sstrs[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL
} ;
 
const char *ustrs[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL
} ;
 
const char *vstrs[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL
} ;


void
init_cmdline_opts()
{
  opts[(int) O_MWN]   = "-Wn" ;
  opts[(int) O_PWN]   = "+Wn" ;
  opts[(int) O_T]     = "-T" ;
  opts[(int) O_R]     = "-r" ;
  opts[(int) O_12]    = "-12" ;
  opts[(int) O_24]    = "-24" ;
  opts[(int) O_ANAL]  = "-analog" ;
  opts[(int) O_DIG]   = "-digital" ;
  opts[(int) O_PDATE] = "+date" ;
  opts[(int) O_MDATE] = "-date" ;
  opts[(int) O_PSEC]  = "+seconds" ;
  opts[(int) O_MSEC]  = "-seconds" ;
  opts[(int) O_V]     = "-v" ;
  opts[(int) O_QUE]   = "-?" ;
  opts[(int) O_HELP]  = "-help" ;
  opts[(int) O_TZ]    = "-TZ" ;
  opts[(int) O_A]     = "-alarm" ;
  opts[(int) O_AT]    = "-alarmtime" ;
  opts[(int) O_AO]    = "Once" ;
  opts[(int) O_AD]    = "Daily" ;
  opts[(int) O_ACMD]  = "-alarmcmd" ;
  opts[(int) O_HCMD]  = "-hourcmd" ;
  opts[(int) O_NAME]  = "-name" ;
}


void
init_text()   /* Setup text strings depending upon language. */
{
  int i ;

/* Alarm settings. */

  astrs[(int) A_NONE]  = "none" ;
  astrs[(int) A_ONCE]  = "once" ;
  astrs[(int) A_DAILY] = "daily" ;

/* Clock resources. */

  clk_res[(int) R_CFACE]   = DGET("faceAnalog") ;
  clk_res[(int) R_IFACE]   = DGET("iconAnalog") ;
  clk_res[(int) R_DIGMODE] = DGET("digital12Hour") ;
  clk_res[(int) R_LOCAL]   = DGET("showLocal") ;
  clk_res[(int) R_SECHAND] = DGET("secondHand") ;
  clk_res[(int) R_DATE]    = DGET("date") ;
  clk_res[(int) R_TZONE]   = DGET("timeZone") ;
  clk_res[(int) R_AHRVAL]  = DGET("alarmHrValue") ;
  clk_res[(int) R_AMINVAL] = DGET("alarmMinValue") ;
  clk_res[(int) R_ACHOICE] = DGET("alarmChoice") ;
  clk_res[(int) R_ACMD]    = DGET("alarmCommand") ;
  clk_res[(int) R_HCMD]    = DGET("hourlyCommand") ;
  clk_res[(int) R_TITLE]   = DGET("hasTitle") ;
  clk_res[(int) R_SFONT]   = DGET("secondsFont") ;
  clk_res[(int) R_DTFONT]  = DGET("dateFont") ;
  clk_res[(int) R_ICOLOR]  = DGET("iconHasWindowColor") ;
  clk_res[(int) R_SLEEP]   = DGET("sleepWhenObscured") ;

/* Days of the week. */
  i = 0 ;
  days[i++] = LGET("Sun") ;
  days[i++] = LGET("Mon") ;
  days[i++] = LGET("Tue") ;
  days[i++] = LGET("Wed") ;
  days[i++] = LGET("Thu") ;
  days[i++] = LGET("Fri") ;
  days[i++] = LGET("Sat") ;

/* Help messages. */

  hstr[(int) H_PFRAME]    = DGET("clock:PropertyFrame") ;
  hstr[(int) H_PPANEL]    = DGET("clock:PropertyPanel") ;
  hstr[(int) H_FCHOICE]   = DGET("clock:FaceStyle") ;
  hstr[(int) H_ICHOICE]   = DGET("clock:IconStyle") ;
  hstr[(int) H_DIGSTYLE]  = DGET("clock:DigStyle") ;
  hstr[(int) H_DISPSTYLE] = DGET("clock:DisplayStyle") ;
  hstr[(int) H_TZSTYLE]   = DGET("clock:TimeZoneStyle") ;
  hstr[(int) H_TZBUT]     = DGET("clock:TimeZoneButton") ;
  hstr[(int) H_TZNAME]    = DGET("clock:TimeZoneName") ;
  hstr[(int) H_SCHOICE]   = DGET("clock:SWatchChoice") ;
  hstr[(int) H_ACHOICE]   = DGET("clock:AlarmChoice") ;
  hstr[(int) H_HRCHOICE]  = DGET("clock:HourChoice") ;
  hstr[(int) H_MINCHOICE] = DGET("clock:MinuteChoice") ;
  hstr[(int) H_ACMD]      = DGET("clock:AlarmCommand") ;
  hstr[(int) H_REPEAT]    = DGET("clock:RepeatChoice") ;
  hstr[(int) H_HCMD]      = DGET("clock:HourCommand") ;
  hstr[(int) H_APPLY]     = DGET("clock:ApplyButton") ;
  hstr[(int) H_RESET]     = DGET("clock:ResetButton") ;
  hstr[(int) H_DEF]       = DGET("clock:DefaultButton") ;
  hstr[(int) H_CANVAS]    = DGET("clock:DisplayCanvas") ;
  hstr[(int) H_FRAME]     = DGET("clock:ClockFrame") ;
  hstr[(int) H_PROPS]     = DGET("clock:Properties") ;

/* Hours. */

  hrstrs[(int) H_AM] = LGET("am") ;
  hrstrs[(int) H_PM] = LGET("pm") ;

/* Messages. */

  mess[(int) M_AVAL]   = MGET("%s: -alarm invalid value [%s]\n") ;
  mess[(int) M_ATVAL]  = MGET("%s -alarmtime invalid time value [%s]\n") ;
  mess[(int) M_RESD]   = MGET("%s: resource %s - invalid value [%d]\n") ;
  mess[(int) M_RESS]   = MGET("%s: resource %s - invalid value [%s]\n") ;
  mess[(int) M_TZINFO] = LGET("Couldn't get timezone information.\n") ;
  mess[(int) M_FONT]   = MGET("%s: couldn't get the default font.\n") ;
  mess[(int) M_ICON]   = MGET("%s: cannot read icon filename (%s)\n") ;

/* Months. */

  i = 0;
  months[i++] = LGET("Jan") ;
  months[i++] = LGET("Feb") ;
  months[i++] = LGET("Mar") ;
  months[i++] = LGET("Apr") ;
  months[i++] = LGET("May") ;
  months[i++] = LGET("Jun") ;
  months[i++] = LGET("Jul") ;
  months[i++] = LGET("Aug") ;
  months[i++] = LGET("Sep") ;
  months[i++] = LGET("Oct") ;
  months[i++] = LGET("Nov") ;
  months[i++] = LGET("Dec") ;

/*  Days of the month.
 * 
 *  [12/10/91, wmui]
 *  NOTE: Need to translate these "numerials" because display size conflict with
 *        month translations.
 */

  i = 0 ;

/*  STRING_EXTRACTION SUNW_DESKSET_CLOCK_LABEL
 * 
 *  The following are days of the month.  If local custom is to use numerials
 *  as days of the month, then just translate them as is.
 *  For example:  msgid "1"
 *                msgstr "1"
 *  Otherwise, please provide the appropriate translation.
 */

  month_days[i++] = LGET("1") ;
  month_days[i++] = LGET("2") ;
  month_days[i++] = LGET("3") ;
  month_days[i++] = LGET("4") ;
  month_days[i++] = LGET("5") ;
  month_days[i++] = LGET("6") ;
  month_days[i++] = LGET("7") ;
  month_days[i++] = LGET("8") ;
  month_days[i++] = LGET("9") ;
  month_days[i++] = LGET("10") ;
  month_days[i++] = LGET("11") ;
  month_days[i++] = LGET("12") ;
  month_days[i++] = LGET("13") ;
  month_days[i++] = LGET("14") ;
  month_days[i++] = LGET("15") ;
  month_days[i++] = LGET("16") ;
  month_days[i++] = LGET("17") ;
  month_days[i++] = LGET("18") ;
  month_days[i++] = LGET("19") ;
  month_days[i++] = LGET("20") ;
  month_days[i++] = LGET("21") ;
  month_days[i++] = LGET("22") ;
  month_days[i++] = LGET("23") ;
  month_days[i++] = LGET("24") ;
  month_days[i++] = LGET("25") ;
  month_days[i++] = LGET("26") ;
  month_days[i++] = LGET("27") ;
  month_days[i++] = LGET("28") ;
  month_days[i++] = LGET("29") ;
  month_days[i++] = LGET("30") ;
  month_days[i++] = LGET("31") ;

/* Scales. */

  sstrs[(int) S_SMALL]      = LGET("small") ;
  sstrs[(int) S_MEDIUM]     = LGET("medium") ;
  sstrs[(int) S_LARGE]      = LGET("large") ;
  sstrs[(int) S_EXTRALARGE] = LGET("extra_large") ;

/* Usage message. */

  i = 0 ;
  ustrs[i++] = MGET("%s: version 3.2.%1d\n\n") ;
  ustrs[i++] = LGET("Usage: %s [-Wn] [+Wn] [-T] [-TZ timezone] [-12] [-24]\n") ;
  ustrs[i++] = LGET("\t[-alarm setting] [-alarmtime hr:min] [-alarmcmd cmd]\n") ;
  ustrs[i++] = LGET("\t[-analog] [-digital] [+date] [-date] [-help] [-hourcmd cmd]\n") ;
  ustrs[i++] = LGET("\t[-name app-name] [-r] [+seconds] [-seconds] [-v] [-?]\n") ;
  ustrs[i++] = LGET("\nKeyboard accelerators:\n") ;
  ustrs[i++] = LGET("\t1 - set 12hr mode if digital display.\n") ;
  ustrs[i++] = LGET("\t2 - set 24hr mode if digital display.\n") ;
  ustrs[i++] = LGET("\tc - toggle clock face (analog/digital).\n") ;
  ustrs[i++] = LGET("\td - toggle display of date.\n") ;
  ustrs[i++] = LGET("\ti - toggle icon face (analog/roman).\n") ;
  ustrs[i++] = LGET("\ts - toggle seconds display.\n") ;
  ustrs[i++] = LGET("\tS - stopwatch (reset, start, stop).\n") ;
  ustrs[i++] = LGET("\tt - toggle timezone (local/other).\n") ;
  ustrs[i++] = LGET("\tT - toggle test mode.\n") ;
  ustrs[i++] = LGET("\tq - quit the clock\n") ;

/* Various */

  vstrs[(int) V_TRUE]   = "true" ;
  vstrs[(int) V_FALSE]  = "false" ;
  vstrs[(int) V_TZ]     = DGET("TZ=%s") ;
  vstrs[(int) V_LABEL]  = LGET("clock") ;
  vstrs[(int) V_MTITLE] = LGET("Clock") ;
  vstrs[(int) V_MSTR]   = LGET("Properties...") ;
}
