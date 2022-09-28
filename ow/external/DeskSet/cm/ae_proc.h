/*
 * static  char sccsid[] = "@(#)ae_proc.h 1.5 92/11/16 Copyr 1991 Sun Microsystems, Inc.";
 * as_proc.h
 */

#ifndef ae_proc_h
#define ae_proc_h

extern char *get_appt();
extern void check_dnd_fullness(Miniappt *);
extern void set_default_values(ae_window_objects *);
extern void ae_make_menus();
extern int save_appt(char *);
extern void update_props();

#endif
