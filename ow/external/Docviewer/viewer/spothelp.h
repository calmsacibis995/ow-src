#ifndef	_SPOTHELP_H_
#define	_SPOTHELP_H_

#ident "@(#)spothelp.h	1.13 06/10/93 Copyright 1990 Sun Microsystems, Inc."

extern char *make_help_message(char* s);

// Definitions for Open Look "spot help" keys in the viewer

// Spot help for the frame & icon
#define	VIEWER_FRAME_HELP		make_help_message("frame")
#define	VIEWER_ICON_HELP		make_help_message("icon")

// Spot help for the main control panel and its buttons
#define	CONTROL_PANEL_HELP		make_help_message("controlPanel")
#define	VIEW_BUTTON_HELP		make_help_message("viewButton")
#define	PRINT_BUTTON_HELP		make_help_message("printButton")
#define	PGFWD_BUTTON_HELP		make_help_message("pgfwdButton")
#define	GOBACK_BUTTON_HELP		make_help_message("gobackButton")
#define	PGBWD_BUTTON_HELP		make_help_message("pgbwdButton")

// Spot help for the menu items attached to the viewButton
#define	PARTPAGE_MENUITEM_HELP		make_help_message("partialPageMenuItem")
#define	FULLPAGE_MENUITEM_HELP		make_help_message("fullPageMenuItem")
#define	SHOWLINKS_MENUITEM_HELP		make_help_message("showLinksMenuItem")
#define	HIDELINKS_MENUITEM_HELP		make_help_message("hideLinksMenuItem")
#define	FOL_LINK_MENUITEM_HELP		make_help_message("followLinkMenuItem")
#define	PAGEINFO_MENUITEM_HELP		make_help_message("pageInfoMenuItem")
#define	STD_SIZE_MENUITEM_HELP		make_help_message("stdSizeMenuItem")
#define	CUSTOM_MAG_MENUITEM_HELP	make_help_message("customMagnificationMenuItem")

// Spot help for the main canvas
#define	VIEWER_CANVAS_HELP		make_help_message("canvas")
#define	VIEWER_SCROLLBAR_HELP		make_help_message("scrollBar")

// Spot help for the canvas menu
#define NEXTPAGE_MENUITEM_HELP		make_help_message("nextPageMenuItem")
#define PREVPAGE_MENUITEM_HELP		make_help_message("prevPageMenuItem")
#define REDISPLAY_MENUITEM_HELP		make_help_message("redisplayMenuItem")
#define GOBACK_MENUITEM_HELP		make_help_message("goBackMenuItem")


// Spot help for the Custom Magnification Sheet items
#define	MAG_PANEL_HELP			make_help_message("magnifyPanel")
#define	MAGNIFY_SLIDER_HELP		make_help_message("magnifySlider")
#define	APPLY_BUTTON_HELP		make_help_message("applyButton")
#define	RESET_BUTTON_HELP		make_help_message("resetButton")

// Spot help for the Page Info window items
#define	PAGEINFO_PANEL_HELP		make_help_message("pageInfoPanel")

// Spot help for 'Print' popup window.
//
#define	PRINT_HELP			make_help_message("printwin")
#define	PRINT_LIST_HELP			make_help_message("printwin_list")
#define	PRINT_COPIES_HELP		make_help_message("printwin_copies")
#define	PRINT_TOTAL_PAGES_HELP		make_help_message("printwin_total_pages")
#define	PRINT_DESTINATION_HELP		make_help_message("printwin_destination")
#define	PRINT_PATHNAME_HELP		make_help_message("printwin_pathname")
#define	PRINT_FILENAME_HELP		make_help_message("printwin_filename")
#define	PRINT_PRINTER_NAME_HELP		make_help_message("printwin_printer_name")
#define	PRINT_PAGE_ORDER_HELP		make_help_message("printwin_page_order")

#endif	/* _SPOTHELP_H_ */
