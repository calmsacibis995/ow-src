/*
static  char sccsid[] = "@(#)cm_tty.h 1.6 94/05/06 Copyr 1991 Sun Microsystems, Inc.";
*/

typedef enum {
        minsunit = 0,
        hrsunit = 1,
        daysunit = 2
}Advunit;

typedef struct {
        char    *beep_advance;
        char    *flash_advance;
        char    *open_advance;
        char    *mail_advance;
        int     beep_on;
        int     flash_on;
        int     open_on;
        int     mail_on;
        Advunit beep_unit;
        Advunit flash_unit;
        Advunit open_unit;
        Advunit mail_unit;
	int time_format;
	char 	*mail_to;
	Separator_Type	separator;
	Ordering_Type	ordering;
} Insert_Info;

