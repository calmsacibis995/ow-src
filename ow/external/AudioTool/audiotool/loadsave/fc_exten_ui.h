#ifndef	fc_exten_HEADER
#define	fc_exten_HEADER

/*
 * fc_exten_ui.h - User interface object and function declarations.
 * This file was generated by `gxv' from `fc_exten.G'.
 * DO NOT EDIT BY HAND.
 */


extern Attr_attribute	INSTANCE;

extern Xv_opaque	fc_exten_format_menu_create(caddr_t, Xv_opaque);
extern Xv_opaque	fc_exten_compress_menu_create(caddr_t, Xv_opaque);

typedef struct {
	Xv_opaque	panel;
	Xv_opaque	controls;
	Xv_opaque	format_button;
	Xv_opaque	format_msg;
	Xv_opaque	fmt_button_group;
	Xv_opaque	format;
	Xv_opaque	fmt_group;
	Xv_opaque	compress_button;
	Xv_opaque	compress_msg;
	Xv_opaque	compress_button_group;
	Xv_opaque	compression;
	Xv_opaque	compress_group;
	Xv_opaque	size_msg;
	Xv_opaque	file_size;
	Xv_opaque	file_size_group;
	Xv_opaque	fmt_compress_group;
	Xv_opaque	show_files;
	Xv_opaque	main_group;
} fc_exten_panel_objects;

extern fc_exten_panel_objects	*fc_exten_panel_objects_initialize(fc_exten_panel_objects *, Xv_opaque);

extern Xv_opaque	fc_exten_panel_panel_create(fc_exten_panel_objects *, Xv_opaque);
extern Xv_opaque	fc_exten_panel_controls_create(fc_exten_panel_objects *, Xv_opaque);
extern Xv_opaque	fc_exten_panel_format_button_create(fc_exten_panel_objects *, Xv_opaque);
extern Xv_opaque	fc_exten_panel_format_msg_create(fc_exten_panel_objects *, Xv_opaque);
extern Xv_opaque	fc_exten_panel_fmt_button_group_create(fc_exten_panel_objects *, Xv_opaque);
extern Xv_opaque	fc_exten_panel_format_create(fc_exten_panel_objects *, Xv_opaque);
extern Xv_opaque	fc_exten_panel_fmt_group_create(fc_exten_panel_objects *, Xv_opaque);
extern Xv_opaque	fc_exten_panel_compress_button_create(fc_exten_panel_objects *, Xv_opaque);
extern Xv_opaque	fc_exten_panel_compress_msg_create(fc_exten_panel_objects *, Xv_opaque);
extern Xv_opaque	fc_exten_panel_compress_button_group_create(fc_exten_panel_objects *, Xv_opaque);
extern Xv_opaque	fc_exten_panel_compression_create(fc_exten_panel_objects *, Xv_opaque);
extern Xv_opaque	fc_exten_panel_compress_group_create(fc_exten_panel_objects *, Xv_opaque);
extern Xv_opaque	fc_exten_panel_size_msg_create(fc_exten_panel_objects *, Xv_opaque);
extern Xv_opaque	fc_exten_panel_file_size_create(fc_exten_panel_objects *, Xv_opaque);
extern Xv_opaque	fc_exten_panel_file_size_group_create(fc_exten_panel_objects *, Xv_opaque);
extern Xv_opaque	fc_exten_panel_fmt_compress_group_create(fc_exten_panel_objects *, Xv_opaque);
extern Xv_opaque	fc_exten_panel_show_files_create(fc_exten_panel_objects *, Xv_opaque);
extern Xv_opaque	fc_exten_panel_main_group_create(fc_exten_panel_objects *, Xv_opaque);
#endif
