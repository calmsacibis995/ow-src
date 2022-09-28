/* sccsid[] = "@(#)log.h 3.2 92/07/09 Copyr 1988 Sun Micro"; */

/* log.h */


#define VERSION		1
#define DEFAULT_LOG	"callog"
#define DEFAULT_BAK	".calbak"
#define DEFAULT_TMP	".caltmp"
#define	DEFAULT_DIR	"/usr/spool/calendar"
#define	DEFAULT_MODE	(S_IRUSR|S_IRGRP|S_IWGRP)

extern int daemon_gid, daemon_uid;

extern char *find_directory(/* char *who */);

extern int start_log (/* char *target; char *file */);

extern char *get_tmp(/* char *who */);

extern char *get_bak(/* char *who */);

extern char *get_log(/* char *who */);

extern int append_log (/* char *file; Appt *appt; Transaction action; */);

extern int append_access_log (/* char *file; int *types; Access_Entry *list[]; */);

extern int create_log(/* char *owner, char *file */);

