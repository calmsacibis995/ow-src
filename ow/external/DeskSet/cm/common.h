/* static  char sccsid[] = "@(#)common.h 1.9 93/05/03 Copyr 1991 Sun Microsystems, Inc.";

*/

#if LATER
typedef struct {
        int data_version;
        char* target;
} Cdata;

extern Cdata *get_cdata();
extern void map_name(), free_cd_from_blist();
#endif
extern void reset_values(), backup_values(), reset_alarm(), blist_write_names();
extern Boolean duplicate_cd();
extern char *get_appt_str();
extern void common_update_lists();
