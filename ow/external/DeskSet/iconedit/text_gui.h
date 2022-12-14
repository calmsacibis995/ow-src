#ifndef	text_HEADER
#define	text_HEADER

/*
 * text_ui.h - User interface object and function declarations.
 * This file was generated by `gxv' from `./text.G'.
 * DO NOT EDIT BY HAND.
 */

extern Attr_attribute	INSTANCE;


typedef struct {
	Xv_opaque	popup;
	Xv_opaque	controls;
	Xv_opaque	text;
	Xv_opaque	font_family_setting;
	Xv_opaque	font_size_setting;
	Xv_opaque	font_style_setting;
	Xv_opaque	font_weight_setting;
} text_popup_objects;

extern text_popup_objects	*text_popup_objects_initialize();

extern Xv_opaque	text_popup_popup_create();
extern Xv_opaque	text_popup_controls_create();
extern Xv_opaque	text_popup_text_create();
extern Xv_opaque	text_popup_font_family_setting_create();
extern Xv_opaque	text_popup_font_size_setting_create();
extern Xv_opaque	text_popup_font_style_setting_create();
extern Xv_opaque	text_popup_font_weight_setting_create();
#endif
