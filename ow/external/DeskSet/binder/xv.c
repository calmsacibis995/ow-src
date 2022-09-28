#ifndef lint
#ifdef sccs
static char sccsid[]="@(#)xv.c	1.13 12/15/93 Copyright 1987-1990 Sun Microsystems, Inc.";
#endif   
#endif
/*
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


#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>

#include <xview/xview.h>
#include <xview/cms.h>
#include <xview/panel.h>
#include <xview/svrimage.h>
#include <xview/font.h>

#include "ds_popup.h"
#include "xv.h"
#include "binder_tt.h"
#include "props.h"

#define TEXT_LENGTH     20
#define CHIP_HEIGHT	16
#define CHIP_WIDTH	16

Server_image    fg_chip;
Server_image    bg_chip;

static int      row = 0;
Xv_opaque 	pmenu;


/*
 * Create XViewInfo object that will store the defaults for this
 * application.
 * Should we be reading from xdefaults to get geometry size, etc?
 */

XviewInfo *
init_xview ()
{
    XviewInfo	*xv_defaults;

    xv_defaults = (XviewInfo *) calloc (1, sizeof (XviewInfo));

    xv_defaults->frame_label_set = FALSE;
    xv_defaults->frame_label = (char *) NULL;
    xv_defaults->icon_label_set = FALSE;
    xv_defaults->icon_label = (char *) NULL;
    xv_defaults->geometry_set = FALSE;
    xv_defaults->icon_font = (char *) NULL;
   
    return (xv_defaults);
}

/*
 * Create object `view_menu' in the specified instance.
 */
/*ARGSUSED*/
static Xv_opaque
binder_view_menu_create( ip )
	Binder *ip;
{
	extern void	view_all();
	extern void	view_shared();
	extern void	view_personal();
	Menu	        obj;

	obj = xv_create(XV_NULL, MENU,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:ViewButton",
		MENU_ACTION_ITEM, MGET( "All Entries" ), view_all,
		MENU_ACTION_ITEM, MGET( "Shared Entries" ), view_shared,
		MENU_ACTION_ITEM, MGET( "Personal Entries" ), view_personal,
		NULL);

	return obj;
}

/*
 * Create object `props_menu' in the specified instance.
 */
/*ARGSUSED*/
Xv_opaque
binder_props_menu_create( ip )
	Binder *ip;
{
	Menu	        obj;

	obj = menu_create (
		MENU_APPEND_ITEM, menu_create_item(
			MENU_ACTION_ITEM, MGET("Icon..."), show_icon_props,
#ifdef KYBDACC
			MENU_ACCELERATOR, "coreset Props",
#endif
			XV_HELP_DATA, "binder:PropsButton",
			NULL),
		MENU_APPEND_ITEM, menu_create_item(
			MENU_ACTION_ITEM, MGET("File Types..."), show_binding_props,
			XV_HELP_DATA, "binder:PropsButton",
			NULL),
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:PropsButton",
		NULL);

	return obj;
}

/*
 * Create object `frame' in the specified instance.
 */
static Xv_opaque
binder_frame_create( ip, owner )
	Binder         *ip;
	Xv_opaque	owner;
{
	Xv_opaque		obj;
	Xv_opaque		frame_image;
	static unsigned short	frame_bits[] = {
#include "binder.icon"
	};
	Xv_opaque		frame_image_mask;
	static unsigned short	frame_mask_bits[] = {
#include "binder.mask.icon"
	};
        char                   *s1, *s2;
        char                   *relname, *hostname;
        char                   *label;
	Icon			frame_icon;
        extern char            *ds_relname();
        extern char            *ds_hostname();
	
	frame_image = xv_create(XV_NULL, SERVER_IMAGE,
		SERVER_IMAGE_BITS, frame_bits,
		SERVER_IMAGE_DEPTH, 1,
		XV_WIDTH, 64,
		XV_HEIGHT, 64,
		NULL);
	frame_image_mask = xv_create(XV_NULL, SERVER_IMAGE,
		SERVER_IMAGE_BITS, frame_mask_bits,
		SERVER_IMAGE_DEPTH, 1,
		XV_WIDTH, 64,
		XV_HEIGHT, 64,
		NULL);

        s1 = MGET("binder");
        frame_icon = xv_create (NULL, ICON,
                        ICON_IMAGE, frame_image,
                        ICON_MASK_IMAGE, frame_image_mask,
                        ICON_TRANSPARENT, TRUE,
			WIN_RETAINED, TRUE,
                        XV_LABEL, s1,
			NULL),

	obj = xv_create(owner, FRAME,
		XV_KEY_DATA, INSTANCE, ip,
		FRAME_CLOSED, FALSE,
		FRAME_SHOW_FOOTER, TRUE,
		FRAME_ICON, frame_icon,
		FRAME_SHOW_RESIZE_CORNER, TRUE,
#ifdef OW_I18N
		WIN_USE_IM,	FALSE,
#endif
		NULL);
        s2 = MGET("Binder");
        relname  = ds_relname();
        hostname = ds_hostname( xv_get( obj, XV_DISPLAY ) );
        label = ( char * )malloc( strlen( relname ) + strlen( hostname) + strlen( s2 ) + 14 );
        sprintf( label, "%s %s%s", s2, relname, hostname );

        xv_set( obj, FRAME_LABEL, label, 
		     /*FRAME_ICON, frame_icon, */
		     NULL );

	return obj;
}

/*
 * Create object `panel' in the specified instance.
 */
static Xv_opaque
binder_panel_create( ip, owner )
	Binder         *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:BinderPanel",
		XV_X, 0,
		XV_Y, 0,
		PANEL_ITEM_X_GAP, 5,
		WIN_BORDER, FALSE,
		WIN_CONSUME_EVENTS, WIN_LEFT_KEYS, LOC_DRAG, NULL,
                PANEL_ACCEPT_KEYSTROKE, TRUE,
		NULL);
	return obj;
}

/*
 * Create object `save_button' in the specified instance.
 */
static Xv_opaque
binder_save_button_create( ip, owner )
	Binder         *ip;
	Xv_opaque	owner;
{
        extern void     save_button();
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, xv_col( ip->panel, 0 ),
		XV_Y, xv_row( ip->panel, 0 ),
		XV_HELP_DATA, "binder:SaveButton",
		PANEL_LABEL_STRING,  MGET("Save") ,
		PANEL_NOTIFY_PROC, save_button,
		NULL);
	return obj;
}

/*
 * Create object `view_button' in the specified instance.
 */
static Xv_opaque
binder_view_button_create( ip, owner )
	Binder         *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, xv_get( ip->save_button, XV_X ) +
		      xv_get( ip->save_button, XV_WIDTH ) + 
		      xv_get( ip->panel, PANEL_ITEM_X_GAP ),
		XV_Y, xv_row( owner, 0 ),
		XV_HELP_DATA, "binder:ViewButton",
		PANEL_ITEM_MENU, binder_view_menu_create( ip ),
		PANEL_LABEL_STRING,  MGET("View")  ,
		NULL);
	return obj;
}

/*
 * Create object `undo_button' in the specified instance.
 */
static Xv_opaque
binder_undo_button_create( ip, owner )
	Binder         *ip;
	Xv_opaque	owner;
{
        extern void     undo_button();
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, xv_get( ip->view_button, XV_X ) +
		      xv_get( ip->view_button, XV_WIDTH ) +
		      xv_get( ip->panel, PANEL_ITEM_X_GAP ),
		XV_Y, xv_row( owner, 0 ),
		XV_HELP_DATA, "binder:UndoButton",
		PANEL_LABEL_STRING,  MGET("Undo")  ,	
		PANEL_NOTIFY_PROC, undo_button,
 	        NULL);
	return obj;
}

/*
 * Create object `props_button' in the specified instance.
 */
static Xv_opaque
binder_props_button_create( ip, owner )
	Binder	       *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	pmenu = binder_props_menu_create (ip);

	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, xv_get( ip->undo_button, XV_X ) +
		      xv_get( ip->undo_button, XV_WIDTH ) + 
		      xv_get( ip->panel, PANEL_ITEM_X_GAP ),
		XV_Y, xv_row( owner, 0 ),
		XV_HELP_DATA, "binder:PropsButton",
		PANEL_ITEM_MENU, pmenu,
		PANEL_LABEL_STRING,  MGET("Props")  ,
		NULL);
#ifdef KYBDACC
	xv_set (ip->frame, FRAME_MENU_ADD, pmenu, NULL);
#endif
	return obj;
}

/*
 * Create object `tns_list' in the specified instance.
 */
static Xv_opaque
binder_tns_list_create( ip, owner )
	Binder         *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
        extern void     tns_list_notify();
	
	obj = xv_create(owner, PANEL_LIST,
		PANEL_LIST_DISPLAY_ROWS, 1,
		PANEL_LIST_ROW_HEIGHT,   LIST_ENTRY_HEIGHT,
		PANEL_LIST_WIDTH,        xv_get( ip->props_button, XV_X ) +
			                 xv_get( ip->props_button, XV_WIDTH ),
		PANEL_LIST_TITLE,        MGET( "Binder Entries" ),
                PANEL_CHOOSE_NONE,       TRUE,
		PANEL_CHOOSE_ONE,    	 TRUE,
		PANEL_READ_ONLY,         TRUE,
		PANEL_NOTIFY_PROC,       tns_list_notify,
		XV_HELP_DATA,            "binder:IconEntries",
		XV_KEY_DATA, INSTANCE, 	 ip,
		XV_X,                    xv_col( ip->panel, 0 ),
		XV_Y,                    xv_row( ip->panel, ++row ),
		NULL);

	return obj;
}

/*
 * Create object `new_button' in the specified instance.
 */
static Xv_opaque
binder_new_button_create(ip, owner)
	Binder         *ip;
	Xv_opaque	owner;
{
        extern void     new_button();
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:NewButton",
		PANEL_LABEL_STRING,  MGET("New") ,
		PANEL_NOTIFY_PROC, new_button,
		NULL);
	return obj;
}

/*
 * Create object `dup_button' in the specified instance.
 */
static Xv_opaque
binder_dup_button_create( ip, owner )
        Binder         *ip;
	Xv_opaque	owner;
{
        extern void     dup_button();
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:DuplicateButton",
		PANEL_LABEL_STRING, MGET("Duplicate") ,
		PANEL_NOTIFY_PROC, dup_button,
		NULL);
	return obj;
}

/*
 * Create object `delete_button' in the specified instance.
 */
static Xv_opaque
binder_delete_button_create( ip, owner )
	Binder         *ip;
	Xv_opaque	owner;
{
        extern void     delete_button();
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:DeleteButton",
		PANEL_LABEL_STRING, MGET("Delete") ,
		PANEL_NOTIFY_PROC, delete_button,
		NULL);
	return obj;
}

/*
 * Create object `props_frame' in the specified instance.
 */
static Xv_opaque
props_frame_create( ip, owner )
	Binder         *ip;
	Xv_opaque	owner;
{
	Xv_opaque		obj;

	obj = xv_create(owner, FRAME_CMD,
		XV_LABEL, MGET("Binder: Properties "),
/*	        WIN_ROW_GAP, 9, */
                FRAME_CMD_PUSHPIN_IN, TRUE,
		XV_KEY_DATA, INSTANCE, ip,
		FRAME_SHOW_FOOTER, TRUE,
                FRAME_DONE_PROC, props_done_proc,
	        XV_HELP_DATA, "binder:PropsFrame",
#ifdef OW_I18N
		WIN_USE_IM, TRUE,
#endif
		NULL );
	return obj;
}

/*
 * Create object `props_panel' in the specified instance.
 */
static Xv_opaque
props_panel_create( ip, owner )
	Binder         *ip;
	Xv_opaque	owner;
{
	Xv_opaque   obj;

	obj = xv_get( owner, FRAME_CMD_PANEL );
        xv_set( obj, XV_KEY_DATA, INSTANCE, ip,
                     XV_HEIGHT, xv_get( ip->panel, XV_HEIGHT ),
	             XV_HELP_DATA, "binder:PropsPanel",
	             NULL );
	return obj;
}

/*
 * Create object `props_category_item' in the specified instance.
 */
static Xv_opaque
props_category_item_create( ip, owner )
	Binder         *ip;
	Xv_opaque	owner;
{
	Xv_opaque    obj;

	obj = xv_create(owner, PANEL_CHOICE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA,  "binder:PropsCategory",
                XV_X, xv_col( owner, 0 ),
                XV_Y, xv_row( owner, 0 ),
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_CHOICE_NROWS, 1,
		PANEL_LABEL_STRING,  MGET( "Category:" ),
		PANEL_NOTIFY_PROC, props_category_notify,
		PANEL_CHOICE_STRINGS,
			 MGET(" Icon ") ,
			 MGET("File Types") ,
			0,
		NULL);
	return obj;
}

/*
 * Create object `props_icon_label' in the specified instance.
 */
static Xv_opaque
props_icon_label_create( ip, owner )
	Binder         *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:Icon",
                PANEL_LABEL_BOLD, TRUE,
		PANEL_LABEL_STRING,  MGET("Icon:"),
		PANEL_VALUE_Y, xv_row( owner, row ),
		NULL);
	return obj;
}

/*
 * Create object `props_preview_image' in the specified instance.
 */
static Xv_opaque
props_icon_preview_create( ip, owner )
	Binder         *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	Server_image    icon_image;

        icon_image = ( Server_image )xv_create( XV_NULL, SERVER_IMAGE,
		XV_WIDTH, MAX_ICON_WIDTH,
		XV_HEIGHT, MAX_ICON_HEIGHT,
		SERVER_IMAGE_DEPTH, ( int )xv_get( ip->frame, WIN_DEPTH ),
                XV_HELP_DATA, "binder:Icon",
		NULL);
        obj = xv_create( owner, PANEL_MESSAGE,
                PANEL_LABEL_IMAGE, icon_image,
                XV_HELP_DATA, "binder:Icon",
		NULL);
	return obj;
}


/*
 * Create object `props_icon_item' in the specified instance.
 */
static Xv_opaque
props_icon_item_create( ip, owner )
	Binder         *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TEXT,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:Icon",
		PANEL_VALUE_Y, xv_row( owner, row ),
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_VALUE_DISPLAY_LENGTH, TEXT_LENGTH,
		PANEL_VALUE_STORED_LENGTH, MAXPATHLEN,
		PANEL_VALUE_Y, xv_row( owner, row ),
                PANEL_NOTIFY_PROC, icon_label_notify,
                PANEL_NOTIFY_LEVEL, PANEL_ALL,
		NULL);
        row++;
	return obj;
}

/*
 * Create object `props_image_file_item' in the specified instance.
 */
static Xv_opaque
props_image_file_item_create( ip, owner )
	Binder         *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TEXT,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:ImageFile",
		PANEL_LABEL_STRING, MGET("Image File:") ,
		PANEL_VALUE_Y, xv_row( owner, row ),
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_VALUE_DISPLAY_LENGTH, TEXT_LENGTH,
		PANEL_VALUE_STORED_LENGTH, MAXPATHLEN,
                PANEL_NOTIFY_PROC, image_file_notify,
                PANEL_NOTIFY_LEVEL, PANEL_ALL,
		PANEL_READ_ONLY, FALSE,
		NULL);
        row++;
	return obj;
}

/*
 * Create object `props_mask_file_item' in the specified instance.
 */
static Xv_opaque
props_mask_file_item_create( ip, owner )
	Binder         *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TEXT,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:MaskFile",
		PANEL_LABEL_STRING, MGET("Mask File:") ,
		PANEL_VALUE_Y, xv_row( owner, row ),
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_VALUE_DISPLAY_LENGTH, TEXT_LENGTH,
		PANEL_VALUE_STORED_LENGTH, MAXPATHLEN,
                PANEL_NOTIFY_PROC, mask_file_notify,
                PANEL_NOTIFY_LEVEL, PANEL_ALL,
		PANEL_READ_ONLY, FALSE,
		NULL);
	row++;
	return obj;
}

/*
 * Create object `props_fg_color_label' in the specified instance.
 */
static Xv_opaque
props_fg_color_label_create( ip, owner )
	Binder         *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:ForegrColor",
                PANEL_LABEL_BOLD, TRUE,
		PANEL_LABEL_STRING, MGET("Foregr Color:") ,
		PANEL_VALUE_Y, xv_row( owner, row ),
		NULL);
       return obj;
}

/*
 * Create object `props_fg_color_item' in the specified instance.
 */
static Xv_opaque
props_fg_color_item_create( ip, owner )
	Binder         *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;


	obj = xv_create(owner, PANEL_TEXT,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:ForegrColor",
		PANEL_VALUE_Y, xv_row( owner, row ),
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_VALUE_DISPLAY_LENGTH, TEXT_LENGTH,
		PANEL_VALUE_STORED_LENGTH, MAXPATHLEN,
		PANEL_VALUE_Y, xv_row( owner, row ),
                PANEL_NOTIFY_PROC, fg_color_notify,
                PANEL_NOTIFY_LEVEL, PANEL_ALL,
		NULL);
	row++;
	return obj;
}


/*
 * Create object `props_bg_color_label' in the specified instance.
 */
static Xv_opaque
props_bg_color_label_create( ip, owner )
	Binder         *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:BackgrColor",
                PANEL_LABEL_BOLD, TRUE,
		PANEL_LABEL_STRING, MGET("Backgr Color:") ,
		PANEL_VALUE_Y, xv_row( owner, row ),	
          	NULL);
       return obj;
}

/*
 * Create object `props_bg_color_item' in the specified instance.
 */
static Xv_opaque
props_bg_color_item_create( ip, owner )
	Binder         *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;


	obj = xv_create(owner, PANEL_TEXT,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:BackgrColor",
		PANEL_VALUE_Y, xv_row( owner, row ),
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_VALUE_DISPLAY_LENGTH, TEXT_LENGTH,
		PANEL_VALUE_STORED_LENGTH, MAXPATHLEN,
		PANEL_VALUE_Y, xv_row( owner, row ),
                PANEL_NOTIFY_PROC, bg_color_notify,
                PANEL_NOTIFY_LEVEL, PANEL_ALL,
		NULL);
	row++;
	return obj;
}

/*
 * Create object `props_fg_color_button' in the specified instance.
 */
static Xv_opaque
props_fg_color_button_create( ip, owner )
	Binder	       *ip;
	Xv_opaque	owner;
{
        Props          *p = ( Props * )ip->properties;
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:ForegrButton",
	        XV_Y, xv_get( p->fg_color_item, XV_Y ),
		PANEL_LABEL_STRING,  MGET("..."),
#ifdef TOOLTALK
		PANEL_NOTIFY_PROC, b_fgcolor_tt,
#endif
		NULL);
	return obj;
}

/*
 * Create object `props_bg_color_button' in the specified instance.
 */
static Xv_opaque
props_bg_color_button_create( ip, owner )
	Binder	       *ip;
	Xv_opaque	owner;
{
        Props          *p = ( Props * )ip->properties;
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:BackgrButton",
	        XV_Y, xv_get( p->bg_color_item, XV_Y ),
		PANEL_LABEL_STRING,  MGET("..."),
#ifdef TOOLTALK
		PANEL_NOTIFY_PROC, b_bgcolor_tt,
#endif		
		NULL);
	return obj;
}

/*
 * Create object `props_fg_color_chip' in the specified instance.
 */
Xv_opaque
props_fg_color_chip_create( ip, owner )
	Binder	        *ip;
	Xv_opaque	owner;
{
        Props          *p = ( Props * )ip->properties;
	Xv_opaque	obj;
	
        if ( !fg_chip )
          fg_chip = ( Server_image )xv_create( XV_NULL, SERVER_IMAGE,
			   XV_WIDTH, CHIP_WIDTH,
			   XV_HEIGHT, CHIP_HEIGHT,
			   SERVER_IMAGE_DEPTH, ( int )xv_get( ip->frame, WIN_DEPTH ),
			   NULL);
				
        obj = xv_create(owner, PANEL_MESSAGE,
			PANEL_LABEL_IMAGE, fg_chip,
			XV_Y, xv_get( p->fg_color_item, XV_Y ),
		        XV_KEY_DATA, INSTANCE, ip,
		        XV_HELP_DATA, "binder:ForegrColor",
			NULL);

	return obj;
}

/*
 * Create object `props_bg_color_chip' in the specified instance.
 */
Xv_opaque
props_bg_color_chip_create( ip, owner )
 	Binder          *ip;
	Xv_opaque       owner;
{
        Props          *p = ( Props * )ip->properties;
	Xv_opaque	obj;
	
        if ( !bg_chip )
          bg_chip = ( Server_image )xv_create( XV_NULL, SERVER_IMAGE,
			   XV_WIDTH, CHIP_WIDTH,
			   XV_HEIGHT, CHIP_HEIGHT,
			   SERVER_IMAGE_DEPTH, ( int )xv_get( ip->frame, WIN_DEPTH ),
			   NULL);
				  
        obj = xv_create(owner, PANEL_MESSAGE,
			PANEL_LABEL_IMAGE, bg_chip,
			XV_Y, xv_get( p->bg_color_item, XV_Y ),
		        XV_KEY_DATA, INSTANCE, ip,	
		        XV_HELP_DATA, "binder:BackgrColor",
		        NULL);

	return obj;
}

/*
 * Create object `props_open_item' in the specified instance.
 */
static Xv_opaque
props_open_item_create( ip, owner )
	Binder         *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TEXT,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:Application",
		PANEL_LABEL_STRING,  MGET("Application:") ,
		PANEL_VALUE_Y, xv_row( owner, row ),
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_VALUE_DISPLAY_LENGTH, TEXT_LENGTH,
		PANEL_VALUE_STORED_LENGTH, MAXPATHLEN,
                PANEL_NOTIFY_PROC, application_props_notify,
                PANEL_NOTIFY_LEVEL, PANEL_ALL, 
 		PANEL_READ_ONLY, FALSE,
		NULL);
	row++;
	return obj;
}


/*
 * Create object `props_print_item' in the specified instance.
 */
static Xv_opaque
props_print_item_create( ip, owner )
	Binder         *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TEXT,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:PrintMethod",
		PANEL_LABEL_STRING,  MGET("Print Method:") ,
		PANEL_VALUE_Y, xv_row( owner, row ),
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_VALUE_DISPLAY_LENGTH, TEXT_LENGTH,
		PANEL_VALUE_STORED_LENGTH, MAXPATHLEN,
                PANEL_NOTIFY_PROC, icon_props_notify,
                PANEL_NOTIFY_LEVEL, PANEL_ALL,
		PANEL_READ_ONLY, FALSE,
		NULL);
	row++;
	return obj;
}


/*
 * Create object `props_apply_button' in the specified instance.
 */
static Xv_opaque
props_apply_button_create( ip, owner )
	Binder	       *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:ApplyButton",
		PANEL_LABEL_STRING,  MGET("Apply"),
		PANEL_NOTIFY_PROC, apply_button,
		NULL);
	return obj;
}

/*
 * Create object `props_reset_button' in the specified instance.
 */
static Xv_opaque
props_reset_button_create( ip, owner )
	Binder	       *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:ResetButton",
		PANEL_LABEL_STRING,  MGET("Reset"),
		PANEL_NOTIFY_PROC, reset_button,
		NULL);
	return obj;
}

/*
 * Create object `props_plus_button' in the specified instance.
 */
static Xv_opaque
props_plus_button_create( ip, owner )
	Binder	       *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:PlusButton",
		PANEL_LABEL_STRING,  MGET("+"),
		PANEL_NOTIFY_PROC, plus_button,
		NULL);
	return obj;
}

/*
 * Create object `props_reset_button' in the specified instance.
 */
static Xv_opaque
props_minus_button_create( ip, owner )
	Binder	       *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:MinusButton",
		PANEL_LABEL_STRING,  MGET("-"),
		PANEL_NOTIFY_PROC, minus_button,
		NULL);
	return obj;
}


/*
 * Create icon properties objects.
 */
static void
create_icon_props( b )
    Binder    *b;
{
    Props     *p = ( Props * )b->properties;
    int        longest = 0;
    int        x;
    int        font_width, font_height;
    Xv_opaque  left, right;

    left  = p->panels[ICON_PANEL_L];
    right = p->panels[ICON_PANEL_R];

    /*
     * Create objects on Left Side of Props Sheet.
     */
    row = 1;
    p->icon_label        = props_icon_label_create( b, left );
    p->icon_preview      = props_icon_preview_create( b, left );
    p->icon_item         = props_icon_item_create( b, left );
    p->image_file_item   = props_image_file_item_create( b, left );
    p->mask_file_item    = props_mask_file_item_create( b, left );
    p->fg_color_label    = props_fg_color_label_create( b, left );
    p->fg_color_item     = props_fg_color_item_create( b, left );
    p->bg_color_label    = props_bg_color_label_create( b, left );
    p->bg_color_item     = props_bg_color_item_create( b, left );
    p->fg_color_button   = props_fg_color_button_create( b, left );
    p->bg_color_button   = props_bg_color_button_create( b, left );
    p->fg_color_chip     = props_fg_color_chip_create( b, left );
    p->bg_color_chip     = props_bg_color_chip_create( b, left );
    p->icon_apply_button = props_apply_button_create( b, left );
    p->icon_reset_button = props_reset_button_create( b, left );
    p->icon_plus_button  = props_plus_button_create( b, left );

    /*
     * Create objects on Right Side of Props Sheet.
     */
    row = 1;
    p->open_item         = props_open_item_create( b, right );
    p->print_item        = props_print_item_create( b, right );
    p->icon_minus_button = props_minus_button_create( b, right );

    /*
     * Determine the longest label on Left Side.
     */
    longest = 0;
    longest = MAX( xv_get( p->icon_label, XV_WIDTH ), longest );
    longest = MAX( xv_get( p->image_file_item, PANEL_LABEL_WIDTH ), longest );
    longest = MAX( xv_get( p->mask_file_item, PANEL_LABEL_WIDTH ), longest );
    longest = MAX( xv_get( p->fg_color_label, XV_WIDTH ), longest );
    longest = MAX( xv_get( p->bg_color_label, XV_WIDTH ), longest );

    /*
     * Set the x position for the text items.
     */
    x = longest + ( 2 * xv_get( left, PANEL_ITEM_X_GAP ) );

    font_width  = ( int )xv_get( xv_get( left, XV_FONT), FONT_DEFAULT_CHAR_WIDTH );
    font_height = ( int )xv_get( xv_get( left, XV_FONT), FONT_DEFAULT_CHAR_HEIGHT );

    xv_set( p->icon_label, XV_X, x - xv_get( p->icon_label, XV_WIDTH ) - font_width, NULL );
    xv_set( p->icon_preview, XV_X, x, 
	                     XV_Y, xv_get( p->icon_label, PANEL_VALUE_Y ) - 
                                   MAX_ICON_HEIGHT + font_height,
	                     NULL ); 
    xv_set( p->icon_item,
              PANEL_VALUE_X, 
	          x + MAX_ICON_WIDTH + font_width,
              PANEL_VALUE_DISPLAY_WIDTH,
                  xv_get( p->icon_item, PANEL_VALUE_DISPLAY_WIDTH ) - 
                  MAX_ICON_WIDTH - font_width, 
              NULL );
    xv_set( p->image_file_item, PANEL_VALUE_X, x, NULL );
    xv_set( p->mask_file_item, PANEL_VALUE_X, x, NULL );
    xv_set( p->fg_color_label, 
              XV_X, x - xv_get( p->fg_color_label, XV_WIDTH ) - font_width, NULL );
    xv_set( p->fg_color_chip, XV_X, x, NULL );
    xv_set( p->fg_color_item, 
              PANEL_VALUE_X, 
                  x + CHIP_WIDTH + font_width,
              PANEL_VALUE_DISPLAY_WIDTH,
	          xv_get( p->fg_color_item, PANEL_VALUE_DISPLAY_WIDTH ) -
                  CHIP_WIDTH - font_width,
	      NULL );
    xv_set( p->bg_color_label, 
              XV_X, x - xv_get( p->bg_color_label, XV_WIDTH ) - font_width, NULL );
    xv_set( p->bg_color_chip, XV_X, x, NULL );
    xv_set( p->bg_color_item, 
              PANEL_VALUE_X, 
                  x + CHIP_WIDTH + font_width,
              PANEL_VALUE_DISPLAY_WIDTH,
	          xv_get( p->bg_color_item, PANEL_VALUE_DISPLAY_WIDTH ) -
                  CHIP_WIDTH - font_width,
	      NULL );

    /*
     * Set the X position for the (...) buttons.
     * x_pos = PANEL_VALUE_X + PANEL_VALUE_DISPLAY_LENGTH + GAP
     */
    xv_set( p->fg_color_button, XV_X, 
	    xv_get( p->fg_color_item, PANEL_VALUE_X ) +
	    xv_get( p->fg_color_item, PANEL_VALUE_DISPLAY_WIDTH ) +
	    xv_get( left, PANEL_ITEM_X_GAP ), NULL );
    xv_set( p->bg_color_button, XV_X,
	    xv_get( p->bg_color_item, PANEL_VALUE_X ) +
	    xv_get( p->bg_color_item, PANEL_VALUE_DISPLAY_WIDTH ) +
	    xv_get( left, PANEL_ITEM_X_GAP ), NULL );

    /*
     * Determine widest button, Apply/Reset and set both to this width
     * then center them on the left side.
     */
    longest = 0;
    longest = MAX( xv_get( p->icon_apply_button, XV_WIDTH ), longest );
    longest = MAX( xv_get( p->icon_reset_button, XV_WIDTH ), longest );
    xv_set( p->icon_apply_button, PANEL_LABEL_WIDTH,
	                   xv_get( p->icon_apply_button, PANEL_LABEL_WIDTH ) + 
                           ( longest - xv_get( p->icon_apply_button, XV_WIDTH ) ), NULL );
    xv_set( p->icon_reset_button, PANEL_LABEL_WIDTH,
	                   xv_get( p->icon_reset_button, PANEL_LABEL_WIDTH ) + 
                           ( longest - xv_get( p->icon_reset_button, XV_WIDTH ) ), NULL );

    /*
     * Save the widest text item on the Left Side of properties sheet.
     * width = ( XV_X of ... button ) + ( XV_WIDTH of ... button ) + GAP
     */
    p->widest = MAX( xv_get( p->fg_color_button, XV_X ) +
                     xv_get( p->fg_color_button, XV_WIDTH ) +
         	     xv_get( left, PANEL_ITEM_X_GAP ), p->widest );

    /*
     * Determine the longest label on Right Side.
     */
    longest = 0;
    longest = MAX( xv_get( p->open_item, PANEL_LABEL_WIDTH ), longest );
    longest = MAX( xv_get( p->print_item, PANEL_LABEL_WIDTH ), longest );

    /*
     * Set the x position on Right panel.
     */
    x = longest + ( 2 * xv_get( right, PANEL_ITEM_X_GAP ) );
    xv_set( p->open_item, PANEL_VALUE_X, x, NULL );
    xv_set( p->print_item, PANEL_VALUE_X, x, NULL );

    /*
     * Now save the width of the Right Side.
     */
    p->widest = MAX( xv_get( p->open_item, PANEL_VALUE_X ) +
                      xv_get( p->open_item, PANEL_VALUE_DISPLAY_WIDTH ) +
                      2 * xv_get( right, PANEL_ITEM_X_GAP ), p->widest );

}

/*
 * Create object `props_fns_list' in the specified instance.
 */
static Xv_opaque
props_fns_list_create(ip, owner)
	Binder         *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
        extern void     fns_list_notify();
	
	obj = xv_create(owner, PANEL_LIST,
		PANEL_LIST_DISPLAY_ROWS, 8,
		PANEL_LIST_WIDTH,        200,
                PANEL_CHOOSE_NONE,       TRUE,
		PANEL_CHOOSE_ONE,    	 TRUE,
		PANEL_READ_ONLY,         TRUE,
		PANEL_NOTIFY_PROC,       fns_list_notify,
		XV_HELP_DATA,            "binder:FileEntries",
		XV_KEY_DATA, INSTANCE, 	 ip,
		XV_X, 		         xv_col( owner, 1 ),
		XV_Y, 		         xv_row( owner, row ),
		NULL);
	return obj;
}

/*
 * Create object `props_pattern_item' in the specified instance.
 */
static Xv_opaque
props_pattern_item_create(ip, owner)
	Binder         *ip;
	Xv_opaque	owner;
{
        Props          *p = ( Props * )ip->properties;
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TEXT,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:Pattern",
		PANEL_LABEL_STRING,  MGET("Pattern:") ,
		XV_X, xv_col( owner, 1 ),
		PANEL_VALUE_Y, xv_get( p->fns_list, XV_Y ) +
			       xv_get( p->fns_list, XV_HEIGHT ) +
			       xv_get( p->panel, PANEL_ITEM_Y_GAP ),
		PANEL_NOTIFY_PROC, pattern_notify,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_VALUE_DISPLAY_LENGTH, 17,
		PANEL_VALUE_STORED_LENGTH, MAXPATHLEN,
		PANEL_READ_ONLY, FALSE,
		NULL);
	return obj;
}

/*
 * Create object `props_new_button' in the specified instance.
 */
static Xv_opaque
props_new_button_create(ip, owner)
	Binder         *ip;
	Xv_opaque	owner;
{
        Props          *p = ( Props * )ip->properties;
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, xv_get( p->fns_list, XV_X ) +
		      xv_get( p->fns_list, XV_WIDTH ) +
		      xv_get( xv_get( p->fns_list, PANEL_LIST_SCROLLBAR ), XV_WIDTH ),
		XV_Y, xv_row( owner, row ),
		XV_HELP_DATA, "binder:PropsNewButton",
		PANEL_LABEL_STRING,  MGET("New") ,
		PANEL_NOTIFY_PROC, props_new_button,
		NULL);
        row++;
	return obj;
}

/*
 * Create object `props_cut_button' in the specified instance.
 */
static Xv_opaque
props_cut_button_create(ip, owner)
	Binder         *ip;
	Xv_opaque	owner;
{
        Props          *p = ( Props * )ip->properties;
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, xv_get( p->new_button, XV_X ),
		XV_Y, xv_row( owner, row ),
		XV_HELP_DATA, "binder:CutButton",
		PANEL_LABEL_STRING,  MGET("Cut") ,
		PANEL_NOTIFY_PROC, cut_button,
		NULL);
        row++;
	return obj;
}

/*
 * Create object `props_copy_button' in the specified instance.
 */
static Xv_opaque
props_copy_button_create(ip, owner)
	Binder         *ip;
	Xv_opaque	owner;
{
        Props          *p = ( Props * )ip->properties;
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, xv_get( p->new_button, XV_X ),
		XV_Y, xv_row( owner, row ),
		XV_HELP_DATA, "binder:CopyButton",
		PANEL_LABEL_STRING,  MGET("Copy") ,
		PANEL_NOTIFY_PROC, copy_button,
		NULL);
        row++;
	return obj;
}

/*
 * Create object `props_paste_button' in the specified instance.
 */
static Xv_opaque
props_paste_button_create(ip, owner)
	Binder         *ip;
	Xv_opaque	owner;
{
        Props          *p = ( Props * )ip->properties;
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, xv_get( p->new_button, XV_X ),
		XV_Y, xv_row( owner, row ),
		XV_HELP_DATA, "binder:PasteButton",
		PANEL_LABEL_STRING,  MGET("Paste") ,
		PANEL_NOTIFY_PROC, paste_button,
		NULL);
        row++;
	return obj;
}

/*
 * Create object `props_delete_button' in the specified instance.
 */
static Xv_opaque
props_delete_button_create(ip, owner)
	Binder         *ip;
	Xv_opaque	owner;
{
        Props          *p = ( Props * )ip->properties;
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, xv_get( p->new_button, XV_X ),
		XV_Y, xv_row( owner, row ),
		XV_HELP_DATA, "binder:PropsDeleteButton",
		PANEL_LABEL_STRING,  MGET("Delete") ,
		PANEL_NOTIFY_PROC, props_delete_button,
		NULL);
        row++;
	return obj;
}

/*
 * Create object `props_identify_item' in the specified instance.
 */
Xv_opaque
props_identify_item_create(ip, owner)
	Binder         *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_CHOICE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:Identify",
		PANEL_VALUE_Y, xv_row( owner, row ),
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_CHOICE_NROWS, 1,
		PANEL_LABEL_STRING,  MGET("Identify:") ,
		PANEL_NOTIFY_PROC, identify_notify,
		PANEL_CHOICE_STRINGS,
			 MGET("by Name") ,
			 MGET("by Content") ,
			0,
		NULL);
        row++;
	return obj;
}

/*
 * Create object `props_offset_item' in the specified instance.
 */
Xv_opaque
props_offset_item_create(ip, owner)
	Binder         *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_NUMERIC_TEXT,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA,  "binder:TagOffset",
		PANEL_NOTIFY_PROC, binding_props_notify,
                PANEL_NOTIFY_LEVEL, PANEL_ALL,
		PANEL_LABEL_STRING,  MGET("Tag Offset:"),
                PANEL_INACTIVE, TRUE,
		PANEL_MAX_VALUE, 999,
                PANEL_VALUE_DISPLAY_LENGTH, 4,
		PANEL_VALUE_Y, xv_row( owner, row ),
		PANEL_CHOICE_NROWS, 1,
		NULL);
        row++;
	return obj;
}

/*
 * Create object `props_type_item' in the specified instance.
 */
Xv_opaque
props_type_item_create(ip, owner)
	Binder         *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_CHOICE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA,  "binder:TagType",
		PANEL_NOTIFY_PROC, tag_type_notify,
		PANEL_VALUE_Y, xv_row( owner, row ),
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_CHOICE_NROWS, 2,
		PANEL_LABEL_STRING,  MGET("Tag Type:") ,
                PANEL_INACTIVE, TRUE,
		PANEL_CHOICE_STRINGS,
			 MGET("Byte") ,
			 MGET("Short") ,
			 MGET("Long") ,
			 MGET("String") ,
			0,
		NULL);
        row = row + 2;
	return obj;
}


/*
 * Create object `props_value_item' in the specified instance.
 */
static Xv_opaque
props_value_item_create(ip, owner)
	Binder         *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TEXT,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:TagValue",
		PANEL_NOTIFY_PROC, binding_props_notify,
                PANEL_NOTIFY_LEVEL, PANEL_ALL,
		PANEL_LABEL_STRING,  MGET("Tag Value:") ,
                PANEL_INACTIVE, TRUE,
		PANEL_VALUE_Y, xv_row( owner, row ),
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_VALUE_DISPLAY_LENGTH, TEXT_LENGTH,
		PANEL_VALUE_STORED_LENGTH, MAXPATHLEN,
		PANEL_READ_ONLY, FALSE,
		NULL);
	row++;
	return obj;
}

/*
 * Create object `props_mask_item' in the specified instance.
 */
static Xv_opaque
props_mask_item_create(ip, owner)
	Binder         *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TEXT,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "binder:TagMask",
		PANEL_NOTIFY_PROC, binding_props_notify,
                PANEL_NOTIFY_LEVEL, PANEL_ALL,
		PANEL_LABEL_STRING,  MGET("Tag Mask:") ,
		PANEL_VALUE_Y, xv_row( owner, row ),
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_VALUE_DISPLAY_LENGTH, TEXT_LENGTH,
		PANEL_VALUE_STORED_LENGTH, MAXPATHLEN,
                PANEL_INACTIVE, TRUE,
		PANEL_READ_ONLY, FALSE,
		NULL);
	row++;
	return obj;
}

/*
 * Create binding properties objects.
 */
static void
create_binding_props( b )
    Binder    *b;
{
    Props     *p = ( Props * )b->properties;
    Rect      *rect1, *rect2;
    Xv_opaque  left, right;
    int        longest = 0;
    int        x;

    left  = p->panels[BINDING_PANEL_L];
    right = p->panels[BINDING_PANEL_R];

    /*
     * Create Left Side of Binding Props.
     */
    row = 1;
    p->fns_list             = props_fns_list_create( b, left );
    p->pattern_item         = props_pattern_item_create( b, left );
    p->new_button           = props_new_button_create( b, left );
    p->cut_button           = props_cut_button_create( b, left );
    p->copy_button          = props_copy_button_create( b, left );
    p->paste_button         = props_paste_button_create( b, left );
    p->delete_button        = props_delete_button_create( b, left );
    p->binding_apply_button = props_apply_button_create( b, left );
    p->binding_reset_button = props_reset_button_create( b, left );
    p->binding_plus_button  = props_plus_button_create( b, left );

    /*
     * Create Right Side of Binding Props.
     */
    row = 1;
    p->binding_minus_button = props_minus_button_create( b, right );
    p->identify_item        = props_identify_item_create( b, right );
    p->offset_item          = props_offset_item_create( b, right );
    p->type_item            = props_type_item_create( b, right );
    p->value_item           = props_value_item_create( b, right );
    p->mask_item            = props_mask_item_create( b, right );

    /*
     * Determine widest button and set all buttons to this width.
     */
    longest = 0;
    longest = MAX( xv_get( p->new_button, XV_WIDTH ), longest );
    longest = MAX( xv_get( p->cut_button, XV_WIDTH ), longest );
    longest = MAX( xv_get( p->copy_button, XV_WIDTH ), longest );
    longest = MAX( xv_get( p->paste_button, XV_WIDTH ), longest );
    longest = MAX( xv_get( p->delete_button, XV_WIDTH ), longest );
    
    xv_set( p->new_button, PANEL_LABEL_WIDTH,
	                   xv_get( p->new_button, PANEL_LABEL_WIDTH ) + 
                           ( longest - xv_get( p->new_button, XV_WIDTH ) ), NULL );
    xv_set( p->cut_button, PANEL_LABEL_WIDTH,
	                   xv_get( p->cut_button, PANEL_LABEL_WIDTH ) + 
                           ( longest - xv_get( p->cut_button, XV_WIDTH ) ), NULL );
    xv_set( p->copy_button, PANEL_LABEL_WIDTH,
	                   xv_get( p->copy_button, PANEL_LABEL_WIDTH ) + 
                           ( longest - xv_get( p->copy_button, XV_WIDTH ) ), NULL );
    xv_set( p->paste_button, PANEL_LABEL_WIDTH,
	                   xv_get( p->paste_button, PANEL_LABEL_WIDTH ) + 
                           ( longest - xv_get( p->paste_button, XV_WIDTH ) ), NULL );
    xv_set( p->delete_button, PANEL_LABEL_WIDTH,
	                   xv_get( p->delete_button, PANEL_LABEL_WIDTH ) + 
                           ( longest - xv_get( p->delete_button, XV_WIDTH ) ), NULL );

    /*
     * Determine widest button of Apply/Reset and set both
     * to this width than center them on Left side.
     */
    longest = 0;
    longest = MAX( xv_get( p->binding_apply_button, XV_WIDTH ), longest );
    longest = MAX( xv_get( p->binding_reset_button, XV_WIDTH ), longest );
    xv_set( p->binding_apply_button, XV_WIDTH, longest, NULL );
    xv_set( p->binding_reset_button, XV_WIDTH, longest, NULL );
    xv_set( p->binding_apply_button, PANEL_LABEL_WIDTH,
	                   xv_get( p->binding_apply_button, PANEL_LABEL_WIDTH ) + 
                           ( longest - xv_get( p->binding_apply_button, XV_WIDTH ) ), NULL );
    xv_set( p->binding_reset_button, PANEL_LABEL_WIDTH,
	                   xv_get( p->binding_reset_button, PANEL_LABEL_WIDTH ) + 
                           ( longest - xv_get( p->binding_reset_button, XV_WIDTH ) ), NULL );

    /*
     * Save the width of the Left Side
     */
    p->widest = MAX( xv_get( p->new_button, XV_X ) +
                     xv_get( p->new_button, XV_WIDTH ) +
       	             2 * xv_get( left, PANEL_ITEM_X_GAP ), p->widest );

    /*
     * Determine the longest label on the Right Side.
     */
    longest = 0;
    longest = MAX( xv_get( p->identify_item, PANEL_LABEL_WIDTH ), longest );
    longest = MAX( xv_get( p->offset_item, PANEL_LABEL_WIDTH ), longest );
    longest = MAX( xv_get( p->type_item, PANEL_LABEL_WIDTH ), longest );
    longest = MAX( xv_get( p->value_item, PANEL_LABEL_WIDTH ), longest );
    longest = MAX( xv_get( p->mask_item, PANEL_LABEL_WIDTH ), longest );

    /*
     * Set the x positions on the Right panel.
     */
    x = longest + ( 2 * xv_get( right, PANEL_ITEM_X_GAP ) );
    xv_set( p->identify_item, PANEL_VALUE_X, x, NULL );
    xv_set( p->offset_item, PANEL_VALUE_X, x, NULL );
    xv_set( p->type_item, PANEL_VALUE_X, x, NULL );
    xv_set( p->value_item, PANEL_VALUE_X, x, NULL );
    xv_set( p->mask_item, PANEL_VALUE_X, x, NULL );

    /*
     * Determine the longest value lengths on the Right Side
     */
    longest = 0;
    longest = MAX( xv_get( p->value_item, PANEL_VALUE_DISPLAY_WIDTH ), longest );
    longest = MAX( xv_get( p->mask_item, PANEL_VALUE_DISPLAY_WIDTH ), longest );

    rect1     = ( Rect * )xv_get( p->identify_item, PANEL_ITEM_VALUE_RECT );
    longest   = MAX( rect1->r_width, longest );    
    rect2     = ( Rect * )xv_get( p->type_item, PANEL_ITEM_VALUE_RECT );
    longest   = MAX( rect2->r_width, longest );    

    /*
     * Set the display values to the longest length.
     */
    xv_set( p->type_item, PANEL_VALUE_DISPLAY_WIDTH, longest, NULL );
    xv_set( p->value_item, PANEL_VALUE_DISPLAY_WIDTH, longest, NULL );
    xv_set( p->mask_item, PANEL_VALUE_DISPLAY_WIDTH, longest, NULL );

    rect1->r_width = longest;
    rect2->r_width = longest;
    xv_set( p->identify_item, PANEL_ITEM_VALUE_RECT, rect1, NULL );
    xv_set( p->type_item, PANEL_ITEM_VALUE_RECT, rect2, NULL );

    /*
     * Save the width on the Right Side.
     * Fix later to be max of text items or choice items.
     */
    p->widest = MAX( xv_get( p->value_item, PANEL_VALUE_X ) +
                     xv_get( p->value_item, PANEL_VALUE_DISPLAY_WIDTH ) +
	             2 * xv_get( right, PANEL_ITEM_X_GAP ), p->widest );
	
}

/*
 * Create binding properties objects.
 */
static void
set_panel_sizes( b )
    Binder    *b;
{
    Props     *p = ( Props * )b->properties;
    int        i;
    int        delta;

    /*
     * Position the Right panels next to the Left panels.
     */
    xv_set( p->panels[ ICON_PANEL_R ], XV_X, p->widest, NULL );
    xv_set( p->panels[ BINDING_PANEL_R ], XV_X, p->widest, NULL );

    /*
     * Set the width, height of the left, right panels.
     */
    delta = xv_get( p->cut_button, XV_Y ) - xv_get( p->new_button, XV_Y );
    for ( i = 0; i < TOTAL_PANELS; i++ ) {
      xv_set( p->panels[i], XV_WIDTH, p->widest,
 	                    XV_HEIGHT, xv_get( p->panel, XV_HEIGHT ) - delta, NULL );
    }
    window_fit( p->frame );

    if (xview_info->geometry_set == FALSE)
      xv_set( b->frame, XV_HEIGHT, xv_get( p->frame, XV_HEIGHT ), NULL );

    ds_center_items( p->panels[ICON_PANEL_L], 8,
		     p->icon_apply_button, p->icon_reset_button, NULL );
    ds_center_items( p->panels[BINDING_PANEL_L], 8, 
		     p->binding_apply_button, p->binding_reset_button, NULL );

    /*
     * Set X and Y positions of plus/minus buttons.
     */
    xv_set( p->icon_plus_button, 
            XV_X, p->widest -
	          xv_get( p->icon_plus_button, XV_WIDTH ) - 
	          xv_get( p->panel, PANEL_ITEM_X_GAP ),
            XV_Y, xv_get( p->icon_reset_button, XV_Y ), NULL );

    xv_set( p->icon_minus_button,
            XV_X, xv_get( p->panel, PANEL_ITEM_X_GAP ), 
	    XV_Y, xv_get( p->icon_reset_button, XV_Y ), NULL );

    xv_set( p->binding_plus_button, 
            XV_X, p->widest -
	          xv_get( p->binding_plus_button, XV_WIDTH ) - 
	          xv_get( p->panel, PANEL_ITEM_X_GAP ),
            XV_Y, xv_get( p->binding_reset_button, XV_Y ), NULL );
    xv_set( p->binding_minus_button,
            XV_X, xv_get( p->panel, PANEL_ITEM_X_GAP ), 
	    XV_Y, xv_get( p->binding_reset_button, XV_Y ), NULL );

    
}

static void
create_base_window( b )
    Binder *b;
{
    /*
     * Build all the objects.
     */
    if ( !b->frame ) {
      b->frame = binder_frame_create( b, NULL );
      b->panel = binder_panel_create( b, b->frame );
      b->save_button = binder_save_button_create( b, b->panel );
      b->view_button = binder_view_button_create( b, b->panel );
      b->undo_button = binder_undo_button_create( b, b->panel );
      b->props_button = binder_props_button_create( b, b->panel );
      b->tns_list = binder_tns_list_create( b, b->panel);
      b->new_button = binder_new_button_create( b, b->panel );
      b->dup_button = binder_dup_button_create( b, b->panel );
      b->delete_button = binder_delete_button_create( b, b->panel );
    }

    /*
     * Set the width/height of the panel and center the buttons.
     */
    xv_set( b->panel, XV_WIDTH, 
	   xv_get( xv_get( b->tns_list, PANEL_LIST_SCROLLBAR ), XV_WIDTH ) +
	   xv_get( b->tns_list, PANEL_LIST_WIDTH ) +
	   xv_get( b->panel, PANEL_ITEM_X_GAP ), NULL );
    ds_center_items( b->panel, 9, b->new_button, 
		     b->dup_button,
		     b->delete_button, NULL );
    xv_set( b->panel, XV_HEIGHT,
	   xv_get( b->new_button, XV_Y ) +
	   xv_get( b->new_button, XV_HEIGHT ) +
	   xv_get( b->panel, PANEL_ITEM_Y_GAP ), NULL );
/*
 * Set a min width/height on the frame.
 */
    xv_set (b->frame, FRAME_MIN_SIZE,
	              xv_get (b->props_button, XV_X) +
		      xv_get (b->props_button, XV_WIDTH ) + 
		      xv_get (b->panel, PANEL_ITEM_Y_GAP ),
	              xv_get (b->tns_list, XV_Y) +
	              (xv_get (b->tns_list, PANEL_LIST_ROW_HEIGHT) * 3) +
	              xv_get (b->dup_button, XV_HEIGHT) +
		      xv_get (b->panel, PANEL_ITEM_Y_GAP ) * 2,
	    NULL);
    
    if (xview_info->geometry_set == FALSE)
      window_fit( b->frame );

}

static void
create_props_window( b )
    Binder *b;
{
    Props *p = ( Props * )b->properties;
    int    i;

    if ( !p->frame ) {
      p->frame = props_frame_create( b, b->frame );
      p->panel = props_panel_create( b, p->frame );
      p->category_item = props_category_item_create( b, p->panel );

      for ( i = 0; i < TOTAL_PANELS; i++ ) {
        p->panels[i] = xv_create( p->frame, PANEL,
				  WIN_BORDER, TRUE,
				  WIN_BELOW, p->panel,
				  XV_X, 0,
				  XV_Y, xv_row( p->panel, 1 ), 
		                  XV_KEY_DATA, INSTANCE, b,
		                  XV_HELP_DATA, "binder:PropsPanel",
				  XV_SHOW, FALSE,
				  NULL );
      }
      p->full_size = FALSE;
      p->current_view = ICON_PROPS;

      create_icon_props( b );
      create_binding_props( b );

      set_panel_sizes( b );

    }

}

/*
 * Initialize an instance of object `frame'.
 */
Binder *
binder_objects_initialize( ip )
    Binder  *ip;
{
    Props *p;
    Xv_Font font;

    /*
     * Make the base window.
     */
    if ( !ip && !( ip = ( Binder * )calloc( 1, sizeof( Binder ) ) ) )
      return ( Binder * ) NULL;
    
    create_base_window( ip );
    
    /*
     * Make the props window.
     */
    p = ( Props * )calloc( 1, sizeof( Props ) );
    if ( !p )
      return (Binder *) NULL;
    ip->properties = ( Xv_opaque )p;
    
    create_props_window( ip );
    
    if (xview_info->frame_label_set == TRUE)
	xv_set (ip->frame, FRAME_LABEL, xview_info->frame_label, NULL);

    if (xview_info->icon_label_set == TRUE)
	xv_set (xv_get (ip->frame, FRAME_ICON), 
	        XV_LABEL, xview_info->icon_label, NULL);

   
    if (xview_info->icon_font == NULL)
		if (defaults_exists("icon.font.name", "Icon.Font.Name"))
                        xview_info->icon_font = strdup((char *)
				defaults_get_string( "icon.font.name", 
						"Icon.Font.Name", ""));

    if (xview_info->icon_font != NULL) {
	font = (Xv_Font) xv_find(ip->frame, FONT,
                       FONT_NAME, xview_info->icon_font,
                       0) ;
	if (font != 0)
		xv_set (xv_get (ip->frame, FRAME_ICON), ICON_FONT, font, NULL);
    }

    return( ip );

}


