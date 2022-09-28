/*
 * file_ui.c - User interface object initialization functions.
 * This file was generated by `gxv' from `./file.G'.
 * DO NOT EDIT BY HAND.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/icon_load.h>
#include <xview/panel.h>
#include <xview/scrollbar.h>
#include <xview/svrimage.h>
#include <xview/termsw.h>
#include <xview/text.h>
#include <xview/tty.h>
#include <xview/xv_xrect.h>
#include <xview/file_chsr.h>
#include "file_gui.h"

extern char dir[];

extern int file_dialog_save();
extern int file_dialog_load();

Xv_opaque format_item;

Xv_opaque 
format_menu_create(parent)
    Xv_opaque parent;
{
	extern Menu		save_file();
	extern Menu_item	save_icon();
	extern Menu_item	save_xbm();
	extern Menu_item	save_c_xpm();
	extern Menu_item	save_m_xpm();
        Xv_opaque obj;
    
obj = xv_create(NULL, MENU_COMMAND_MENU,
		MENU_GEN_PROC, save_file,
		MENU_ITEM,
			MENU_STRING,  gettext("XView Icon") ,
			MENU_GEN_PROC, save_icon,
			XV_HELP_DATA, "iconedit:XvIconMenuItem",
			NULL,
		MENU_ITEM,
			MENU_STRING,  gettext("X Bitmap") ,
			MENU_GEN_PROC, save_xbm,
			XV_HELP_DATA, "iconedit:XBMMenuItem",
			NULL,
		MENU_ITEM,
			MENU_STRING,  gettext("Color X Pixmap") ,
			MENU_GEN_PROC, save_c_xpm,
			XV_HELP_DATA, "iconedit:ColorXPMMenuItem",
			NULL,
		MENU_ITEM,
			MENU_STRING,  gettext("Mono X Pixmap") ,
			MENU_GEN_PROC, save_m_xpm,
			XV_HELP_DATA, "iconedit:MonoXPMMenuItem",
			NULL,
		NULL);
    return obj;
}
int
file_dialog_resize_func(fc, frame_rect, exten_rect, left_edge, right_edge, max_height)
        File_chooser fc;
        Rect        *frame_rect;
        Rect        *exten_rect;
        int          left_edge;
        int          right_edge;
        int          max_height;
{
    xv_set(format_item, 
	   XV_X, xv_get(xv_get(fc, FILE_CHOOSER_CHILD,
			       FILE_CHOOSER_FILE_LIST_CHILD),
			XV_X),
	   XV_Y, exten_rect->r_top,
	   PANEL_PAINT, PANEL_NONE,
	   0);

     return -1;
}
Frame
file_dialog_initialize(owner, type)
	Frame	owner;
        int      type;
{
    File_chooser chooser;
    Xv_opaque controls;
    Rect *format_item_rect;

    switch (type) {

    case FILE_CHOOSER_OPEN:
	chooser = xv_create(owner, FILE_CHOOSER_OPEN_DIALOG,
			    FILE_CHOOSER_NOTIFY_FUNC, file_dialog_load,
			    WIN_USE_IM, TRUE,
                            0);
	break;
    case FILE_CHOOSER_SAVEAS:
	chooser = xv_create(owner, FILE_CHOOSER_SAVEAS_DIALOG,
			    FILE_CHOOSER_NOTIFY_FUNC, file_dialog_save,
			    WIN_USE_IM, TRUE,
                            0);
	controls = xv_get(chooser, FRAME_CMD_PANEL);
	xv_set(controls, 
	       XV_X, 0,
	       XV_Y, 0,
	       XV_WIDTH, WIN_EXTEND_TO_EDGE,
	       XV_HEIGHT, WIN_EXTEND_TO_EDGE,
	       WIN_BORDER, FALSE,
	       NULL);
	format_item = xv_create(controls, PANEL_CHOICE,
				PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
				PANEL_LAYOUT, PANEL_HORIZONTAL,
				PANEL_LABEL_STRING, gettext("File format: "),
				XV_HELP_DATA, "iconedit:FileFormat",
				PANEL_CHOICE_STRING, 
				0, gettext("XView Icon"),
				PANEL_CHOICE_STRING, 
				1, gettext("X Bitmap"),
				PANEL_CHOICE_STRING, 
				2, gettext("Color X Pixmap"),
				PANEL_CHOICE_STRING, 
				3, gettext("Mono X Pixmap"),
				0);
	xv_set(format_item, PANEL_VALUE, 0, 0);
	format_item_rect = xv_get(format_item, XV_RECT);
	xv_set(chooser, 
	       FILE_CHOOSER_EXTEN_FUNC,   file_dialog_resize_func,
	       FILE_CHOOSER_EXTEN_HEIGHT, format_item_rect->r_height,
	       NULL);

	break;
    }
    return((Frame)chooser);
}

