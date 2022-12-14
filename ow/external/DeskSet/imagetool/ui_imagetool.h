
/*
 * @(#)ui_imagetool.h 1.40 93/05/25
 *
 * Copyright (c) 1992 - Sun Microsystems Inc.
 *
 */

#ifndef	imagetool_ui_HEADER
#define	imagetool_ui_HEADER

#include <xview/xview.h>
#ifdef FILECHOOSER
#include <xview/file_chsr.h>
#endif

/*
 * imagetool_ui.h - User interface object and function declarations.
 * This file was generated by `gxv' from `imagetool.G'.
 * DO NOT EDIT BY HAND.
 */

/* File Menu menu items... */
#define OPENFC 		1
#define OPENAS		2
#define SAVE		4
#define SAVEAS		5
#define SAVESELECTIONAS 6
#define SAVEPAGEASIMAGE 7
#define PRINTONE        9
#define PRINTPREVIEW	10
#define PRINTD	        11

/* View Menu menu items... */
#define IMAGEINFO	1
#define PAGEVIEW	3
#define PSOPTIONS	4

/* Edit Menu menu items... */
#define	UNDO		1
#define	PALETTE		3
#define PROPERTIES	4

/* Defaults for Print preview pop up */
#define	PRINT_PREVIEW_PAGE_FACTOR 	32

/* Default page size */
#define DEFAULT_PAGE_SIZE	0

/* No header page */
#define NO_HEADER_PAGE	0
#define HEADER_PAGE	1

/* page range choices */
#define PAGE_AS_IMAGE	0
#define ALL_PAGES	1

/* position choices */
#define CENTERED        0
#define MARGINS         1

/* orientation choices */
#define PORTRAIT	0
#define LANDSCAPE	1

/* Default choices for Zoom and Rotate Choices */
#define DEFAULT_ZOOM		100      /* For images only */
#define DEFAULT_ANGLE		90

/* inches/cm/pixel values from choices */
#define INCHES		0
#define CM		1

extern Attr_attribute	INSTANCE;
extern Attr_attribute	DROPSITE;
extern Attr_attribute	SELECTOR;

typedef struct {
	int		frame_label_set;    /* Frame label set on cmd line */
	char	       *frame_label;	    /* Frame label                 */
	int		icon_label_set;	    /* Icon label set on cmd line  */
	char	       *icon_label;	    /* Icon label 		   */
	int		geometry_set;	    /* Geometry set on cmd line    */
	int		depth_set;	    /* Depth set on cmd line       */
	int		depth;	            /* Depth                       */
	int		visual_set;	    /* Visual set on cmd line      */
	int	        visual;	            /* Visual                      */
} XviewInfo;

extern XviewInfo       *xview_info;
extern XviewInfo       *init_xview ();

extern Xv_opaque	file_menu_create();
extern Xv_opaque	edit_menu_create();
extern Xv_opaque	view_menu_create();

typedef struct {
	Xv_opaque	base_window;
	Xv_opaque	base_panel;	
	Xv_opaque	file_menu;
	Xv_opaque	file_button;
	Xv_opaque	edit_menu;
	Xv_opaque	edit_button;
	Xv_opaque	view_menu;
	Xv_opaque	view_button;
        Xv_opaque       help_button;
        Xv_opaque       page_backward_button;
        Xv_opaque       page_forward_button;
	Xv_opaque	dragdrop;
	Xv_opaque	drop_target;
	Xv_opaque	canvas;
	Xv_opaque	hscroll;
	Xv_opaque	vscroll;
	Xv_opaque	ps_canvas;
	Xv_opaque	ps_hscroll;
	Xv_opaque	ps_vscroll;
	int	  	footer_set;	   /* Footer is set		 */
        Rect	        min_size;	   /* Min size of frame          */
        Rect            max_size;          /* Max size of frame          */
} BaseWindowObjects;

extern BaseWindowObjects	*base_window;

extern BaseWindowObjects	*BaseWindowObjectsInitialize();

extern Xv_opaque	base_window_base_window_create();
extern Xv_opaque	base_window_base_panel_create();
extern Xv_opaque	base_window_file_button_create();
extern Xv_opaque	base_window_edit_button_create();
extern Xv_opaque	base_window_view_button_create();
extern Xv_opaque	base_window_help_button_create();
extern Xv_opaque	base_window_page_backward_button_create();
extern Xv_opaque	base_window_page_forward_button_create();
extern Xv_opaque	base_window_dragdrop_create();
extern Xv_opaque	base_window_drop_target_create();
extern Xv_opaque	base_window_canvas_create();
extern Xv_opaque	base_window_hscroll_create();
extern Xv_opaque	base_window_vscroll_create();
extern Xv_opaque	base_window_ps_canvas_create();
extern Xv_opaque	base_window_ps_hscroll_create();
extern Xv_opaque	base_window_ps_vscroll_create();

typedef struct {
	Xv_opaque	open;
	Xv_opaque	controls;
	Xv_opaque	directory;
	Xv_opaque	file;
	Xv_opaque	open_button;
} OpenObjects;

extern OpenObjects	*openf;

extern OpenObjects	*OpenObjectsInitialize();

extern Xv_opaque	open_open_create();
extern Xv_opaque	open_controls_create();
extern Xv_opaque	open_directory_create();
extern Xv_opaque	open_file_create();
extern Xv_opaque	open_open_button_create();

#ifdef FILECHOOSER
extern File_chooser     openfc;
extern File_chooser     OpenfcObjectsInitialize();

extern File_chooser     savefc;
extern File_chooser     SavefcObjectsInitialize();

typedef struct {
       File_chooser     openasfc;
       Xv_opaque        format_list;
} OpenasfcObjects;

extern OpenasfcObjects *openasfc;

extern OpenasfcObjects *OpenasfcObjectsInitialize();

extern File_chooser     openasfc_openasfc_create();
extern Xv_opaque        openasfc_format_list_create();

typedef struct {
       File_chooser     saveasfc;
       Xv_opaque        format_list;
       Xv_opaque        compression;
       Xv_opaque        compression_value;
       Xv_opaque        colors;
       Xv_opaque        colors_value;
} SaveasfcObjects;

extern SaveasfcObjects *saveasfc;

extern SaveasfcObjects *SaveasfcObjectsInitialize();

extern File_chooser     saveasfc_saveasfc_create();
extern Xv_opaque        saveasfc_format_list_create();
extern Xv_opaque        saveasfc_compression_create();
extern Xv_opaque        saveasfc_compression_value_create();
extern Xv_opaque        saveasfc_colors_create();
extern Xv_opaque        saveasfc_colors_value_create();
#endif

typedef struct {
	Xv_opaque	openas;
	Xv_opaque	controls;
	Xv_opaque	format_list;
	Xv_opaque	directory;
	Xv_opaque	file;
	Xv_opaque	open_button;
} OpenasObjects;

extern OpenasObjects	*openas;

extern OpenasObjects	*OpenasObjectsInitialize();

extern Xv_opaque	openas_openas_create();
extern Xv_opaque	openas_controls_create();
extern Xv_opaque	openas_format_list_create();
extern Xv_opaque	openas_directory_create();
extern Xv_opaque	openas_file_create();
extern Xv_opaque	openas_open_button_create();

typedef struct {
	Xv_opaque	saveas;
	Xv_opaque	controls;
	Xv_opaque	format_list;
	Xv_opaque	depth;
	Xv_opaque	depth_value;
	Xv_opaque	compression;
	Xv_opaque	compression_value;
	Xv_opaque	directory;
	Xv_opaque	file;
	Xv_opaque	save_button;
	Xv_opaque	save_selection_button;
} SaveasObjects;

extern SaveasObjects	*saveas;

extern SaveasObjects	*SaveasObjectsInitialize();

extern Xv_opaque	saveas_saveas_create();
extern Xv_opaque	saveas_controls_create();
extern Xv_opaque	saveas_format_list_create();
extern Xv_opaque	saveas_depth_create();
extern Xv_opaque	saveas_depth_value_create();
extern Xv_opaque	saveas_compression_create();
extern Xv_opaque	saveas_compression_value_create();
extern Xv_opaque	saveas_directory_create();
extern Xv_opaque	saveas_file_create();
extern Xv_opaque	saveas_save_button_create();
extern Xv_opaque	saveas_save_selection_button_create();

typedef struct {
        Xv_opaque       print_prev;
        Xv_opaque       page;
        Xv_opaque       controls;
        Xv_opaque       print;
        Xv_opaque       cancel;
} PrintPreviewObjects;

extern PrintPreviewObjects	*print_preview;

extern PrintPreviewObjects	*PrintPreviewObjectsInitialize ();

extern Xv_opaque        print_prev_print_prev_create ();
extern Xv_opaque        print_prev_page_create ();
extern Xv_opaque        print_prev_controls_create ();
extern Xv_opaque        print_prev_print_create ();
extern Xv_opaque        print_prev_cancel_create ();
extern void		resize_print_prev ();

typedef struct {
	Xv_opaque	print;
	Xv_opaque	controls;
	Xv_opaque	copies;
	Xv_opaque	header;
	Xv_opaque	printer;
	Xv_opaque	page_size;
	Xv_opaque	page_range;
	Xv_opaque	size;
	Xv_opaque	size_text;
	Xv_opaque	size_slider;
	Xv_opaque	size_percent;
	Xv_opaque	orientation;
	Xv_opaque	position;
	Xv_opaque	top_margin;
	Xv_opaque	top_margin_text;
	Xv_opaque	left_margin;
	Xv_opaque	left_margin_text;
	Xv_opaque	units;
	Xv_opaque	print_button;
	Xv_opaque	cancel_button;
} PrintObjects;

extern PrintObjects     *print;

extern PrintObjects     *PrintObjectsInitialize();

extern Xv_opaque        print_print_create();
extern Xv_opaque        print_controls_create();
extern Xv_opaque        print_copies_create();
extern Xv_opaque        print_header_create();
extern Xv_opaque        print_printer_create();
extern Xv_opaque        print_page_size_create();
extern Xv_opaque        print_page_range_create();
extern Xv_opaque        print_size_create();
extern Xv_opaque        print_size_text_create();
extern Xv_opaque        print_size_slider_create();
extern Xv_opaque        print_size_percent_create();
extern Xv_opaque        print_orientation_create();
extern Xv_opaque        print_position_create();
extern Xv_opaque        print_top_margin_create();
extern Xv_opaque        print_top_margin_text_create();
extern Xv_opaque        print_left_margin_create();
extern Xv_opaque        print_left_margin_text_create();
extern Xv_opaque        print_units_create();
extern Xv_opaque        print_print_button_create();
extern Xv_opaque        print_cancel_button_create();

typedef struct {
        Xv_opaque       palette;
        Xv_opaque       controls1;
        Xv_opaque       pan;
        Xv_opaque       select;
	Xv_opaque	controls2;
        Xv_opaque       vflip;
        Xv_opaque       hflip;
        Xv_opaque       controls3;
        Xv_opaque       rotate;
        Xv_opaque       rotate_value;
	Xv_opaque	degrees_image;
	Xv_opaque	rotate_degrees;
	Xv_opaque	rotate_slider;
	Xv_opaque	rotate_degrees2;
        Xv_opaque       zoom;
        Xv_opaque       zoom_value;
	Xv_opaque	zoom_percent;
	Xv_opaque	zoom_slider;
	Xv_opaque	zoom_percent2;
        Xv_opaque       controls4;
        Xv_opaque       revert;
} PaletteObjects;

extern PaletteObjects   *palette;

extern PaletteObjects   *PaletteObjectsInitialize();

extern Xv_opaque        palette_palette_create();
extern Xv_opaque        palette_controls1_create();
extern Xv_opaque        palette_pan_create();
extern Xv_opaque        palette_select_create();
extern Xv_opaque        palette_controls2_create();
extern Xv_opaque        palette_vflip_create();
extern Xv_opaque        palette_hflip_create();
extern Xv_opaque        palette_controls3_create();
extern Xv_opaque        palette_rotate_create();
extern Xv_opaque        palette_rotate_value_create();
extern Xv_opaque        palette_rotate_degrees_create();
extern Xv_opaque        palette_rotate_slider_create();
extern Xv_opaque        palette_rotate_degrees2_create();
extern Xv_opaque        palette_zoom_create();
extern Xv_opaque        palette_zoom_value_create();
extern Xv_opaque        palette_zoom_percent_create();
extern Xv_opaque        palette_zoom_slider_create();
extern Xv_opaque        palette_zoom_percent2_create();
extern Xv_opaque        palette_controls4_create();
extern Xv_opaque        palette_revert_create();

typedef struct {
        Xv_opaque        select_cursor;
	Xv_opaque        pan_cursor;
	Xv_opaque	 default_cursor;
} CursorObjects;

extern CursorObjects	*cursor;

extern CursorObjects 	*CursorObjectsInitialize();

extern Xv_opaque 	cursor_select_cursor_create();
extern Xv_opaque 	cursor_default_cursor_create();
extern Xv_opaque 	cursor_pan_cursor_create();

typedef struct {
	Xv_opaque	imageinfo;
	Xv_opaque	controls;
	Xv_opaque	height_label;
	Xv_opaque	height;
	Xv_opaque	width_label;
	Xv_opaque	width;
	Xv_opaque	colors_label;
	Xv_opaque	colors;
	Xv_opaque	size_label;
	Xv_opaque	size;
	Xv_opaque	format_label;
	Xv_opaque	format;
} ImageInfoObjects;

extern ImageInfoObjects	*imageinfo;

extern ImageInfoObjects	*ImageInfoObjectsInitialize();

extern Xv_opaque	imageinfo_imageinfo_create();
extern Xv_opaque	imageinfo_controls_create();
extern Xv_opaque	imageinfo_height_label_create();
extern Xv_opaque	imageinfo_height_create();
extern Xv_opaque	imageinfo_width_label_create();
extern Xv_opaque	imageinfo_width_create();
extern Xv_opaque	imageinfo_colors_label_create();
extern Xv_opaque	imageinfo_colors_create();
extern Xv_opaque	imageinfo_size_label_create();
extern Xv_opaque	imageinfo_size_create();
extern Xv_opaque	imageinfo_format_label_create();
extern Xv_opaque	imageinfo_format_create();

typedef struct {
        Xv_opaque        pageview;
        Xv_opaque        controls1;
        Xv_opaque        start_page;
        Xv_opaque        controls2;
	Xv_opaque        canvas;
        Xv_opaque        vscroll;
        int              width;
        int              height;
	int		 displayed;
        Xv_opaque        pages;
        Xv_opaque        controls3;
        Xv_opaque        end_page;
        Xv_opaque        goto_page;
        Xv_opaque        apply;
        Xv_opaque        cancel;
} PageviewObjects;

extern PageviewObjects *pageview;

extern PageviewObjects *PageviewObjectsInitialize();

extern Xv_opaque pageview_pageview_create();
extern Xv_opaque pageview_controls1_create();
extern Xv_opaque pageview_start_page_create();
extern Xv_opaque pageview_controls2_create();
extern Xv_opaque pageview_canvas_create();
extern Xv_opaque pageview_vscroll_create();
extern Xv_opaque pageview_pages_create();
extern Xv_opaque pageview_controls3_create();
extern Xv_opaque pageview_end_page_create();
extern Xv_opaque pageview_goto_page_create();
extern Xv_opaque pageview_apply_create();
extern Xv_opaque pageview_cancel_create();

typedef struct {
        Xv_opaque       ps_options;
        Xv_opaque       controls;
        Xv_opaque       orientation;
        Xv_opaque       order;
        Xv_opaque       size;
        Xv_opaque       apply;
        Xv_opaque       cancel;
} PsOptionsObjects;

extern PsOptionsObjects      *ps_options;

extern PsOptionsObjects      *PsOptionsObjectsInitialize ();

extern Xv_opaque        ps_options_ps_options_create ();
extern Xv_opaque        ps_options_controls_create ();
extern Xv_opaque        ps_options_orientation_create ();
extern Xv_opaque        ps_options_order_create ();
extern Xv_opaque        ps_options_size_create ();
extern Xv_opaque        ps_options_apply_create ();
extern Xv_opaque        ps_options_cancel_create ();

typedef struct {
	Xv_opaque	props;
	Xv_opaque	controls;
	Xv_opaque	view;
	Xv_opaque	colors;
	Xv_opaque	open_palette;
	Xv_opaque	apply;
	Xv_opaque	reset;
} PropsObjects;

extern PropsObjects	*props;

extern PropsObjects	*PropsObjectsInitialize();

extern Xv_opaque	props_props_create();
extern Xv_opaque	props_controls_create();
extern Xv_opaque	props_view_create();
extern Xv_opaque	props_colors_create();
extern Xv_opaque	props_open_palette_create();
extern Xv_opaque	props_apply_create();
extern Xv_opaque	props_reset_create();

#endif
