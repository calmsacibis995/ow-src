
#ifndef lint
static char sccsid[] = "@(#)propsui.c 1.11 93/07/14";
#endif

/*
 * Copyright (c) 1986, 1987, 1988 by Sun Microsystems, Inc.
 */

/*
 * propsui.c - User interface object initialization functions 
 * for the properties pop up.
 * This file was generated by `gxv' from `imagetool.G'.
 */

#include <sys/param.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include "imagetool.h" 
#include "ui_imagetool.h"

int	panel_x_gap;
int	panel_y_gap;
int	panel_gap;
int	small_panel_gap;

/*
 * Initialize an instance of object `props'.
 */
PropsObjects *
PropsObjectsInitialize(ip, owner)
	PropsObjects	*ip;
	Xv_opaque	owner;
{
        extern void props_size_panel ();
  
	if (!ip && !(ip = (PropsObjects *) calloc(1, sizeof (PropsObjects))))
		return (PropsObjects *) NULL;
	if (!ip->props)
		ip->props = props_props_create(ip, owner);
	if (!ip->controls)
		ip->controls = props_controls_create(ip, ip->props);
	if (!ip->view)
		ip->view = props_view_create(ip, ip->controls);
	if (!ip->colors)
		ip->colors = props_colors_create(ip, ip->controls);
	if (!ip->open_palette)
		ip->open_palette = props_open_palette_create(ip, ip->controls);
	if (!ip->apply)
		ip->apply= props_apply_create(ip, ip->controls);
	if (!ip->reset)
		ip->reset = props_reset_create(ip, ip->controls);

	props_size_panel (ip);

	return ip;
}

void
props_size_panel (ip)
    PropsObjects  *ip;
{
        Rect  *rect;
        int   longest1 = 0, longest2 = 0;
        int   buttons_width;
/*
 * Figure out widest label:
 */
        rect = (Rect *) xv_get (ip->view, PANEL_ITEM_LABEL_RECT);
        longest1 = Max (rect->r_width, longest1);
        rect = (Rect *) xv_get (ip->colors, PANEL_ITEM_LABEL_RECT);
        longest1 = Max (rect->r_width, longest1);
        rect = (Rect *) xv_get (ip->open_palette, PANEL_ITEM_LABEL_RECT);
        longest1 = Max (rect->r_width, longest1);
     
        xv_set (ip->view, PANEL_VALUE_X, longest1 + 25, NULL);
        xv_set (ip->colors, PANEL_VALUE_X, longest1 + 25, NULL);
        xv_set (ip->open_palette, PANEL_VALUE_X, longest1 + 25, NULL);
/*
 * Figure out widest value.
 */
        rect = (Rect *) xv_get (ip->view, PANEL_ITEM_VALUE_RECT);
        longest2 = Max (rect->r_width, longest2);
        rect = (Rect *) xv_get (ip->colors, PANEL_ITEM_VALUE_RECT);
        longest2 = Max (rect->r_width, longest2);
        rect = (Rect *) xv_get (ip->open_palette, PANEL_ITEM_VALUE_RECT);
        longest2 = Max (rect->r_width, longest2);

        buttons_width = xv_get (ip->apply, XV_WIDTH) +
                        xv_get (ip->reset, XV_WIDTH) + 20;

	xv_set (ip->apply, XV_Y, xv_get( ip->open_palette, XV_Y ) +
    			         xv_get( ip->open_palette, XV_HEIGHT ) + 40,
		NULL );
        xv_set( ip->reset, XV_Y, xv_get (ip->apply, XV_Y), NULL );

        xv_set (ip->controls, XV_HEIGHT, xv_get (ip->apply, XV_Y) +
		                         xv_get (ip->apply, XV_HEIGHT) + 25,
		              XV_WIDTH,  longest1 + longest2 + 25 + 25,
			      PANEL_DEFAULT_ITEM, ip->apply,
		              NULL);

        xv_set (ip->apply, XV_X, 
		(xv_get (ip->controls, XV_WIDTH) - buttons_width) / 2,
		NULL);
        xv_set (ip->reset, XV_X, xv_get (ip->apply, XV_X) + 
		                 xv_get (ip->apply, XV_WIDTH) + 20,
		NULL);
        window_fit (ip->props);
}

/*
 * Create object `props' in the specified instance.
 */
Xv_opaque
props_props_create(ip, owner)
	PropsObjects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque            obj;
	extern void          props_done_proc();
	
	obj = xv_create(owner, FRAME_CMD,
		XV_KEY_DATA, INSTANCE, ip,
		XV_LABEL, LGET( "Image Tool:  Properties" ),
		XV_SHOW, FALSE,
		FRAME_SHOW_FOOTER, TRUE,
		FRAME_SHOW_RESIZE_CORNER, FALSE,
		FRAME_CMD_PUSHPIN_IN, TRUE,
		FRAME_DONE_PROC, props_done_proc,
		NULL);
	/*xv_set(xv_get(obj, FRAME_CMD_PANEL), WIN_SHOW, FALSE, NULL); */
	return obj;
}

/*
 * Create object `controls' in the specified instance.
 */
Xv_opaque
props_controls_create(ip, owner)
	PropsObjects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_get( owner, FRAME_CMD_PANEL );
	xv_set( obj,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 0,
		XV_Y, 0,
		XV_WIDTH, WIN_EXTEND_TO_EDGE,
		XV_HEIGHT, WIN_EXTEND_TO_EDGE,
		WIN_BORDER, FALSE,
		XV_HELP_DATA, "imagetool:PropertiesWindow",
		NULL);
        panel_x_gap = xv_get( obj, PANEL_ITEM_X_GAP );
        panel_y_gap = xv_get( obj, PANEL_ITEM_Y_GAP );
        panel_gap = panel_x_gap;
        small_panel_gap = panel_gap - 5;
	return obj;
}

/*
 * Create object `view' in the specified instance.
 */
Xv_opaque
props_view_create(ip, owner)
	PropsObjects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	extern void     props_view_notify();
	
	obj = xv_create(owner, PANEL_CHOICE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, panel_x_gap,
		XV_Y, 25,
		PANEL_CHOICE_NCOLS, 2,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_CHOOSE_NONE, FALSE,
		PANEL_NOTIFY_PROC, props_view_notify,
		PANEL_LABEL_STRING, LGET ("View Image In:"),
		PANEL_CHOICE_STRINGS,
			LGET ("Gray Scale"),
			LGET ("Color"),
			NULL,
		XV_HELP_DATA, "imagetool:PropsView",
		NULL);
	return obj;
}

/*
 * Create object `colors' in the specified instance.
 */
Xv_opaque
props_colors_create(ip, owner)
	PropsObjects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	extern void     props_colors_notify();
	
	obj = xv_create(owner, PANEL_CHOICE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, panel_x_gap,
		XV_Y, xv_get( ip->view, XV_Y ) +
		      xv_get( ip->view, XV_HEIGHT ) +
		      20,
		PANEL_CHOICE_NCOLS, 5,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_CHOOSE_NONE, FALSE,
		PANEL_LABEL_STRING, LGET ("Colors:"),
		PANEL_NOTIFY_PROC, props_colors_notify,
		PANEL_CHOICE_STRINGS,
			LGET ("2 (B&W)"),
			LGET ("16"),
			LGET ("256"),
			LGET ("Thousands"),
			LGET ("Millions"), 
			NULL,
		XV_HELP_DATA, "imagetool:PropsColors",
		NULL);
	return obj;
}

/*
 * Create object `open_palette' in the specified instance.
 */
Xv_opaque
props_open_palette_create(ip, owner)
	PropsObjects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	extern void     props_notify();
	
	obj = xv_create(owner, PANEL_TOGGLE, PANEL_FEEDBACK, PANEL_MARKED,
		XV_KEY_DATA, INSTANCE, ip,
	        XV_X, panel_x_gap,
		XV_Y, xv_get( ip->colors, XV_Y ) +
		      xv_get( ip->colors, XV_HEIGHT ) +
		      30,
		PANEL_CHOICE_NCOLS, 1,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_NOTIFY_PROC, props_notify,
	        PANEL_LABEL_STRING, LGET ("Display Palette:"),
		PANEL_CHOICE_STRING, 0, LGET("On Opening Document"),
		PANEL_VALUE, 1,
		XV_HELP_DATA, "imagetool:PropsOpenPalette",
		NULL);
	return obj;
}


/*
 * Create object `apply' in the specified instance.
 */
Xv_opaque
props_apply_create(ip, owner)
	PropsObjects	*ip;
	Xv_opaque	owner;
{
	extern void     apply_notify_proc();
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, panel_x_gap,
		PANEL_LABEL_STRING, LGET ("Apply"),
		PANEL_NOTIFY_PROC, apply_notify_proc,
		XV_HELP_DATA, "imagetool:PropsApply",
		NULL);
	return obj;
}

/*
 * Create object `reset' in the specified instance.
 */
Xv_opaque
props_reset_create(ip, owner)
	PropsObjects	*ip;
	Xv_opaque	owner;
{
	extern void     reset_notify_proc();
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		PANEL_LABEL_STRING, LGET ("Reset"),
		PANEL_NOTIFY_PROC, reset_notify_proc,
		XV_HELP_DATA, "imagetool:PropsReset",
		NULL);
	return obj;
}

