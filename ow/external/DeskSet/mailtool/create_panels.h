/* Copyright 1992, Sun Microsystems Inc */

#pragma ident "@(#)create_panels.h	1.1 92/12/14 SMI"

/* create_panels.h */

#ifndef mailtool_create_panels_h
#define mailtool_create_panels_h

#include <xview/xview.h>


void mt_resize_filin(Panel, Panel_item);
Menu_item mt_menu_append_blank_item(Menu);
void mt_activate_functions(void);
void mt_deactivate_functions(void);
void mt_create_control_panel(HeaderDataPtr);
void mt_set_undelete_inactive(HeaderDataPtr, int inactive);
Menu generate_headers_popup(HeaderDataPtr);




#ifdef MULTIPLE_FOLDERS
void mt_create_control_panel1(HeaderDataPtr);
#endif



#endif /* mailtool_create_panels_h */
