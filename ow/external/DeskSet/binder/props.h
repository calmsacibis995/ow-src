/*
 *  @(#)props.h	3.4 02/02/93
 *
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

#ifndef	props_HEADER
#define	props_HEADER

#include "binder.h"
#include "xv.h"

#define ICON_PROPS            0
#define BINDING_PROPS         1
#define PROGRAM_PROPS         2

#define BY_NAME               0
#define BY_CONTENT            1

#define BYTE_TAG              0
#define SHORT_TAG             1
#define LONG_TAG              2
#define STRING_TAG            3

#define ICON_PANEL_L          0
#define ICON_PANEL_R          1
#define BINDING_PANEL_L       2
#define BINDING_PANEL_R       3
#define PROGRAM_PANEL_L       4
#define PROGRAM_PANEL_R       5
#define TOTAL_PANELS          6

#define BORDER_WIDTH          1

typedef struct {

     	Xv_opaque	frame;	           /* Props Frame */
     	Xv_opaque	panel;   	   /* Props Panel */
        Xv_opaque       category_item;     /* Props Stack Button */
        Xv_opaque       panels[TOTAL_PANELS];
        int             widest;

        Xv_opaque       icon_label;         /* Icon Props... */
        Xv_opaque       icon_preview;
        Xv_opaque       icon_item;
        Xv_opaque       image_file_item;
        Xv_opaque       mask_file_item;
        Xv_opaque       fg_color_label;
        Xv_opaque       fg_color_item;
        Xv_opaque       bg_color_label;
        Xv_opaque       bg_color_item;
        Xv_opaque       icon_apply_button;
        Xv_opaque       icon_reset_button;
        Xv_opaque       icon_plus_button;
        Xv_opaque       fg_color_chip;
        Xv_opaque       bg_color_chip;
        Xv_opaque       fg_color_button;
        Xv_opaque       bg_color_button;

        Xv_opaque       print_item;
        Xv_opaque       open_item;
        Xv_opaque       icon_minus_button;

        Xv_opaque       fns_list;          /* Binding Props... */
	Xv_opaque       new_button;
	Xv_opaque       cut_button;
	Xv_opaque       copy_button;
	Xv_opaque       paste_button;
	Xv_opaque       delete_button;
        Xv_opaque       pattern_item;
        Xv_opaque       binding_apply_button;
        Xv_opaque       binding_reset_button;
        Xv_opaque       binding_plus_button;

        Xv_opaque       binding_minus_button;
        Xv_opaque       identify_item;
        Xv_opaque       offset_item;
        Xv_opaque       type_item;
        Xv_opaque       value_item;
        Xv_opaque       mask_item;

        int             full_size;
        int             current_view;
 
        Xv_opaque       program_plus_button;
        Xv_opaque       program_minus_button;
  
        int             icon_props_changed;
        int             binding_props_changed;

} Props;

typedef struct {
        Fns_entry  f;
        int        delete;
	int        add;
} Fns_list;

extern void show_icon_props_items();
extern void show_bindings_props_items();
extern void show_program_props_items();
extern void props_category_notify();

extern void show_icon_props();
extern void show_binding_props();
extern void show_program_props();

extern void props_new_button();
extern void cut_button();
extern void copy_button();
extern void paste_button();
extern void props_delete_button();
extern void identify_notify();

extern void apply_button();
extern void reset_button();
extern void plus_button();
extern void minus_button();
extern Notify_value props_done_proc();
extern Panel_setting icon_props_notify();
extern Panel_setting application_props_notify();
extern Panel_setting binding_props_notify();
extern Notify_value pattern_notify();
extern Panel_setting icon_label_notify();
extern Panel_setting image_file_notify();
extern Panel_setting mask_file_notify();
extern Panel_setting fg_color_notify();
extern Panel_setting bg_color_notify();
extern void tag_type_notify();

extern void undisplay_fns_data();
extern void show_binding_items();
extern void show_program_items();
extern void show_icon_items();

#endif
