#ifndef	file_HEADER
#define	file_HEADER

/*
 * file_ui.h - User interface object and function declarations.
 * This file was generated by `gxv' from `./file.G'.
 * DO NOT EDIT BY HAND.
 */

extern Attr_attribute	INSTANCE;

extern Xv_opaque	file_save_format_menu_create();

typedef struct {
	Xv_opaque	popup;
	Xv_opaque	controls;
	Xv_opaque	dir_text;
	Xv_opaque	file_text;
	Xv_opaque	load_bt;
	Xv_opaque	save_bt;
} file_popup_objects;

extern file_popup_objects	*file_popup_objects_initialize();

extern Xv_opaque	file_popup_popup_create();
extern Xv_opaque	file_popup_controls_create();
extern Xv_opaque	file_popup_dir_text_create();
extern Xv_opaque	file_popup_file_text_create();
extern Xv_opaque	file_popup_load_bt_create();
extern Xv_opaque	file_popup_save_bt_create();
#endif
