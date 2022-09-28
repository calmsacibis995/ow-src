/*      @(#)header.h 3.12 IEI SMI      */

/* header.h */

#ifndef __header_h__
#define __header_h__

/*
 * Our long term desire is to eliminate global data, and to allow
 * multiple header windows.  I'm sure that this structure will grow
 * with time; for now it can evolve to track useful data
 */

#include <xview/xview.h>
#include <xview/font.h>
#include "../maillib/obj.h"

struct header_data {
	struct view_window_list *hd_vwl;
	struct folder_obj *hd_folder;
	char 		*hd_folder_name;
        struct msg *hd_curmsg;  /* replaces global mt_curmsg */
        struct header_data *hd_next;  /* for multiple base frames and headers */
	struct reply_panel_data  *hd_rpd_list;
	
	/* Each hd now needs to keep track of its own button and binding list */
	struct button	*hd_sys_button_list;
	struct binding	*hd_binding_list;
	struct binding	*hd_last_binding;

	Xv_opaque	hd_frame;
	Xv_opaque	hd_canvas;
	Xv_opaque	hd_paintwin;
	Xv_opaque	hd_scrollbar;
	Xv_opaque	hd_dropsite;
	Xv_opaque	hd_cmdpanel;
	Xv_Font		hd_textfont;
	Font		hd_fontid;
	Xv_opaque	hd_textfontset;	/* only for I18N level 4 */
	Xv_opaque	hd_screen;
	void		*hd_al;

	/* replace global file_fillin (textfield for mail file */
	Xv_opaque	hd_file_fillin; 

	/* used to hook hd into it, and then passed to different 
		callbacks like mt_prev_proc */
	Menu 		hd_bogus_menu; 
	Menu_item	hd_bogus_item; 

	/* data needed for find functionality.  In a perfect world,
	 * this would be an opaque object
	 */
	Xv_opaque	hd_findframe;
	Xv_opaque	hd_findselect;
	Xv_opaque	hd_findforward;

	Drawable	hd_drawable;
	Drawable	hd_icon_drawable;
	Display		*hd_display;
	GC		hd_gc;
	GC		hd_cleargc;
	GC		hd_icongc;
	GC		hd_attachgc;
	int		hd_foreground;
	int		hd_background;

	int		hd_last_x;	/* position of last mouse click */
	int		hd_last_y;	/* for dnd */

	int		hd_lineheight;	/* how tall is each line */
	int		hd_maxlines;	/* the max # of lines in a canvas */
	char		hd_toomanylines;/* TRUE if there are too many times */
	char		hd_framemapped;	/* Used to reduce extraneous repaints */
	char		hd_looking_for_dnd; /* 2nd click; checking for dnd */
	char		hd_selection_drag;
	char		hd_copy : 1;	/* TRUE if drag for copy */
	char		hd_cache_vw : 1;/* TRUE to cache 1 view window */

	void	(*hd_old_compute_proc)();
};

typedef struct header_data *HeaderDataPtr;

extern int KEY_HEADER_DATA;

#endif	!__header_h__
