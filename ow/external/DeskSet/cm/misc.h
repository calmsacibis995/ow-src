/*
 * static  char sccsid[] = "@(#)misc.h 1.5 93/02/25 Copyr 1991 Sun Microsystems, Inc.";
 * misc.h
 */

#ifndef misc_h
#define misc_h

extern char *periodstr_Cloc[];
extern char *periodstr[];
extern char *repeatstr[];
extern char *repeatval[];

#define RC_FILENAME ".cm.rc"
#define DS_FILENAME ".desksetdefaults"

typedef struct Pentry {
        char    *property_name;
        char    *property_value;
        struct Pentry *next;
} Pentry;

extern Pentry* cm_get_resources();
extern void cm_free_resources();

/* Given Separator_Type, write the separator string in buffer */
extern void	get_separator(/* Separator_Type, char * */);

/* Return a date label */
extern char	*get_datemsg(/* Ordering_Type, Separator_Type */);

/* Write a formatted day string into the buffer */
extern void	format_tick(/* long, Ordering_Type, Separator_Type, char * */);

/* Parse the date string and get the month, day, and year */
extern int	parse_date(/* Ordering_Type, Separator_Type, char *, char *, char *, char * */);

/* Reformat the date string into m/d/y format and write it into the buffer */
extern int	datestr2mdy(/* char *, Ordering_Type, Separator_Type, char * */);

extern void init_periodstr();
extern int pstr_to_unitsCloc();
extern int pstr_to_units();
extern char* period_to_Clocstr();
extern char* period_to_str();
extern int repeatstr_to_Clocinterval();
extern int repeatstr_to_interval();

/* Routines for processing multibyte strings */
extern int cm_mbstrlen();
extern char *cm_mbchar();

#endif
