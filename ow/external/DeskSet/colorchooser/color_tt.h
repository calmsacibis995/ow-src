/*
 * @(#)color_tt.h	1.1 11/9/90
 *

/*
 * tt_props.h - declarations for external interfaces to the props program.
 */

#include "color.h"

extern HSV   xact_work;
extern HSV   xact_win;
extern char *iconname;
extern char *iconmaskname;
extern int   new_image;
extern int   windowmode;

extern void update_colors();
extern void store_custom_colors();
extern void backup_colors();

int  color_tt_init();
void color_tt_quit();
void color_tt_reply();
