/*
 * calctool_ui.c - User interface object initialization functions.
 * This file was generated by `gxv' from `calctool.G'.
 * DO NOT EDIT BY HAND.
 */

/* HA! edited by hand!!! */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/panel.h>
#include <xview/scrollbar.h>
#include <xview/svrimage.h>
#include <xview/termsw.h>
#include <xview/text.h>
#include <xview/tty.h>
#include <xview/xv_xrect.h>
#include <group.h>
#include "calctool_ui.h"

/*
 * Create object `acc_menu' in the specified instance.
 */
Xv_opaque
calctool_acc_menu_create(caddr_t ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(XV_NULL, MENU_COMMAND_MENU,
		XV_KEY_DATA, INSTANCE, ip,
		MENU_TITLE_ITEM, owner ? "" : dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Accuracy"),
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "0 radix places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "1 radix places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "2 radix places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "3 radix places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "4 radix places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "5 radix places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "6 radix places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "7 radix places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "8 radix places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "9 radix places"),
			NULL,
		MENU_GEN_PIN_WINDOW, owner, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Accuracy"),
		NULL);
	return obj;
}

/*
 * Create object `exch_menu' in the specified instance.
 */
Xv_opaque
calctool_exch_menu_create(caddr_t ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(XV_NULL, MENU_COMMAND_MENU,
		XV_KEY_DATA, INSTANCE, ip,
		MENU_TITLE_ITEM, owner ? "" : dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Exchange"),
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 0"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 1"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 2"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 3"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 4"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 5"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 6"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 7"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 8"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 9"),
			NULL,
		MENU_GEN_PIN_WINDOW, owner, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Exchange"),
		NULL);
	return obj;
}

/*
 * Create object `lshift_menu' in the specified instance.
 */
Xv_opaque
calctool_lshift_menu_create(caddr_t ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(XV_NULL, MENU_COMMAND_MENU,
		XV_KEY_DATA, INSTANCE, ip,
		MENU_TITLE_ITEM, owner ? "" : dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Left shift"),
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "1 place"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "2 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "3 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "4 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "5 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "6 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "7 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "8 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "9 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "10 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "11 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "12 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "13 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "14 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "15 places"),
			NULL,
		MENU_GEN_PIN_WINDOW, owner, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Left shift"),
		NULL);
	return obj;
}

/*
 * Create object `base_menu' in the specified instance.
 */
Xv_opaque
calctool_base_menu_create(caddr_t ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(XV_NULL, MENU_COMMAND_MENU,
		XV_KEY_DATA, INSTANCE, ip,
		MENU_TITLE_ITEM, owner ? "" : dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Numeric base"),
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Binary"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Octal"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Decimal"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Hexadecimal"),
			NULL,
		MENU_GEN_PIN_WINDOW, owner, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Numeric base"),
		NULL);
	return obj;
}

/*
 * Create object `disp_menu' in the specified instance.
 */
Xv_opaque
calctool_disp_menu_create(caddr_t ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(XV_NULL, MENU_COMMAND_MENU,
		XV_KEY_DATA, INSTANCE, ip,
		MENU_TITLE_ITEM, owner ? "" : dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Display type"),
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Engineering"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Fixed point"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Scientific"),
			NULL,
		MENU_GEN_PIN_WINDOW, owner, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Display type"),
		NULL);
	return obj;
}

/*
 * Create object `trig_menu' in the specified instance.
 */
Xv_opaque
calctool_trig_menu_create(caddr_t ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(XV_NULL, MENU_COMMAND_MENU,
		XV_KEY_DATA, INSTANCE, ip,
		MENU_TITLE_ITEM, owner ? "" : dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Trigonometric type"),
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Degrees"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Gradients"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Radians"),
			NULL,
		MENU_GEN_PIN_WINDOW, owner, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Trigonometric type"),
		NULL);
	return obj;
}

/*
 * Create object `mode_menu' in the specified instance.
 */
Xv_opaque
calctool_mode_menu_create(caddr_t ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(XV_NULL, MENU_COMMAND_MENU,
		XV_KEY_DATA, INSTANCE, ip,
		MENU_TITLE_ITEM, owner ? "" : dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Mode"),
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Basic"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Financial"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Logical"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Scientific"),
			NULL,
		MENU_GEN_PIN_WINDOW, owner, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Mode"),
		NULL);
	return obj;
}

/*
 * Create object `props_menu' in the specified instance.
 */
Xv_opaque
calctool_props_menu_create(caddr_t ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(XV_NULL, MENU_COMMAND_MENU,
		XV_KEY_DATA, INSTANCE, ip,
		MENU_TITLE_ITEM, owner ? "" : dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Calculator"),
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Properties..."),
			NULL,
		NULL);
	return obj;
}

/*
 * Create object `rcl_menu' in the specified instance.
 */
Xv_opaque
calctool_rcl_menu_create(caddr_t ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(XV_NULL, MENU_COMMAND_MENU,
		XV_KEY_DATA, INSTANCE, ip,
		MENU_TITLE_ITEM, owner ? "" : dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Retrieve"),
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 0"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 1"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 2"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 3"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 4"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 5"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 6"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 7"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 8"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 9"),
			NULL,
		MENU_GEN_PIN_WINDOW, owner, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Retrieve"),
		NULL);
	return obj;
}

/*
 * Create object `sto_menu' in the specified instance.
 */
Xv_opaque
calctool_sto_menu_create(caddr_t ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(XV_NULL, MENU_COMMAND_MENU,
		XV_KEY_DATA, INSTANCE, ip,
		MENU_TITLE_ITEM, owner ? "" : dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Store"),
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 0"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 1"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 2"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 3"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 4"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 5"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 6"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 7"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 8"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Register 9"),
			NULL,
		MENU_GEN_PIN_WINDOW, owner, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Store"),
		NULL);
	return obj;
}

/*
 * Create object `rshift_menu' in the specified instance.
 */
Xv_opaque
calctool_rshift_menu_create(caddr_t ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(XV_NULL, MENU_COMMAND_MENU,
		XV_KEY_DATA, INSTANCE, ip,
		MENU_TITLE_ITEM, owner ? "" : dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Right shift"),
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "1 place"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "2 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "3 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "4 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "5 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "6 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "7 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "8 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "9 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "10 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "11 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "12 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "13 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "14 places"),
			NULL,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "15 places"),
			NULL,
		MENU_GEN_PIN_WINDOW, owner, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Right shift"),
		NULL);
	return obj;
}



/* use command panel rather than ignoring it (as real gxv code does) */
static Xv_opaque
aquire_cmd_panel(void *ip, Xv_opaque owner)
{
    Xv_opaque obj = xv_get(owner, FRAME_CMD_PANEL);
    xv_set(obj, XV_KEY_DATA, INSTANCE, ip, NULL);
    return obj;
}



/*
 * Initialize an instance of object `kframe'.
 */
calctool_kframe_objects *
calctool_kframe_objects_initialize(calctool_kframe_objects *ip, Xv_opaque owner)
{
	if (!ip && !(ip = (calctool_kframe_objects *) calloc(1, sizeof (calctool_kframe_objects))))
		return (calctool_kframe_objects *) NULL;
	if (!ip->kframe)
		ip->kframe = calctool_kframe_kframe_create(ip, owner);
	if (!ip->kcanvas)
		ip->kcanvas = calctool_kframe_kcanvas_create(ip, ip->kframe);
	return ip;
}

/*
 * Create object `kframe' in the specified instance.
 */
Xv_opaque
calctool_kframe_kframe_create(calctool_kframe_objects *ip, Xv_opaque owner)
{
	extern Notify_value	frame_interpose(Xv_window, Event *, Notify_arg, Notify_event_type);
	Xv_opaque	obj;
	Xv_opaque		kframe_image;
	static unsigned short	kframe_bits[] = {
#include "images/calctool.icon"
	};
	Xv_opaque		kframe_image_mask;
	static unsigned short	kframe_mask_bits[] = {
#include "images/calctool.mask.icon"
	};
	
	kframe_image = xv_create(XV_NULL, SERVER_IMAGE,
		SERVER_IMAGE_DEPTH, 1,
		SERVER_IMAGE_BITS, kframe_bits,
		XV_WIDTH, 64,
		XV_HEIGHT, 64,
		NULL);
	kframe_image_mask = xv_create(XV_NULL, SERVER_IMAGE,
		SERVER_IMAGE_DEPTH, 1,
		SERVER_IMAGE_BITS, kframe_mask_bits,
		XV_WIDTH, 64,
		XV_HEIGHT, 64,
		NULL);
	obj = xv_create(owner, FRAME,
		XV_KEY_DATA, INSTANCE, ip,
		XV_WIDTH, 408,
		XV_HEIGHT, 235,
		XV_LABEL, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Calctool"),
		WIN_USE_IM, FALSE,
		FRAME_SHOW_FOOTER, FALSE,
		FRAME_SHOW_RESIZE_CORNER, TRUE,
		FRAME_ICON, xv_create(XV_NULL, ICON,
			ICON_IMAGE, kframe_image,
			ICON_MASK_IMAGE, kframe_image_mask,
			XV_LABEL, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "calculator"),
			NULL),
		NULL);
	xv_set(obj, WIN_CONSUME_EVENTS,
		WIN_MOUSE_BUTTONS,
		LOC_MOVE,
		LOC_DRAG,
		LOC_WINENTER,
		LOC_WINEXIT,
		WIN_ASCII_EVENTS,
		WIN_LEFT_KEYS,
		WIN_RIGHT_KEYS,
		WIN_TOP_KEYS,
		NULL, NULL);
	notify_interpose_event_func(obj,
		(Notify_func) frame_interpose, NOTIFY_SAFE);
	return obj;
}

/*
 * Create object `kcanvas' in the specified instance.
 */
Xv_opaque
calctool_kframe_kcanvas_create(calctool_kframe_objects *ip, Xv_opaque owner)
{
	extern Notify_value	canvas_proc(Xv_window, Event *, Notify_arg, Notify_event_type);
	extern void	canvas_repaint(Canvas, Xv_window, Display *, Window, Xv_xrectlist *);
	extern void	canvas_resize(Canvas, int, int);
	Xv_opaque	obj;
	
	obj = xv_create(owner, CANVAS,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 0,
		XV_Y, 0,
		XV_WIDTH, WIN_EXTEND_TO_EDGE,
		XV_HEIGHT, WIN_EXTEND_TO_EDGE,
		CANVAS_REPAINT_PROC, canvas_repaint,
		CANVAS_X_PAINT_WINDOW, TRUE,
		CANVAS_RESIZE_PROC, canvas_resize,
		NULL);
	xv_set(canvas_paint_window(obj), WIN_CONSUME_EVENTS,
		WIN_MOUSE_BUTTONS,
		LOC_MOVE,
		LOC_DRAG,
		LOC_WINENTER,
		LOC_WINEXIT,
		WIN_ASCII_EVENTS,
		WIN_LEFT_KEYS,
		WIN_RIGHT_KEYS,
		WIN_TOP_KEYS,
		NULL, NULL);
	notify_interpose_event_func(canvas_paint_window(obj),
		(Notify_func) canvas_proc, NOTIFY_SAFE);
	/*
	 * This line is here for backwards compatibility. It will be
	 * removed for the next release.
	 */
	xv_set(canvas_paint_window(obj), XV_KEY_DATA, INSTANCE, ip, NULL);
	return obj;
}

/*
 * Initialize an instance of object `mframe'.
 */
calctool_mframe_objects *
calctool_mframe_objects_initialize(calctool_mframe_objects *ip, Xv_opaque owner)
{
	if (!ip && !(ip = (calctool_mframe_objects *) calloc(1, sizeof (calctool_mframe_objects))))
		return (calctool_mframe_objects *) NULL;
	if (!ip->mframe)
		ip->mframe = calctool_mframe_mframe_create(ip, owner);
	if (!ip->mcanvas)
		ip->mcanvas = calctool_mframe_mcanvas_create(ip, ip->mframe);
	return ip;
}

/*
 * Create object `mframe' in the specified instance.
 */
Xv_opaque
calctool_mframe_mframe_create(calctool_mframe_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, FRAME_CMD,
		XV_KEY_DATA, INSTANCE, ip,
		XV_WIDTH, 403,
		XV_HEIGHT, 66,
		XV_SHOW, FALSE,
		FRAME_SHOW_FOOTER, FALSE,
		FRAME_SHOW_RESIZE_CORNER, FALSE,
		FRAME_CMD_PUSHPIN_IN, TRUE,
		NULL);
	xv_set(xv_get(obj, FRAME_CMD_PANEL), WIN_SHOW, FALSE, NULL);
	return obj;
}

/*
 * Create object `mcanvas' in the specified instance.
 */
Xv_opaque
calctool_mframe_mcanvas_create(calctool_mframe_objects *ip, Xv_opaque owner)
{
	extern Notify_value	canvas_proc(Xv_window, Event *, Notify_arg, Notify_event_type);
	extern void	canvas_repaint(Canvas, Xv_window, Display *, Window, Xv_xrectlist *);
	extern void	canvas_resize(Canvas, int, int);
	Xv_opaque	obj;
	
	obj = xv_create(owner, CANVAS,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 0,
		XV_Y, 0,
		XV_WIDTH, WIN_EXTEND_TO_EDGE,
		XV_HEIGHT, WIN_EXTEND_TO_EDGE,
		CANVAS_REPAINT_PROC, canvas_repaint,
		CANVAS_X_PAINT_WINDOW, TRUE,
		CANVAS_RESIZE_PROC, canvas_resize,
		NULL);
	xv_set(canvas_paint_window(obj), WIN_CONSUME_EVENTS,
		WIN_MOUSE_BUTTONS,
		LOC_MOVE,
		LOC_DRAG,
		LOC_WINENTER,
		LOC_WINEXIT,
		WIN_ASCII_EVENTS,
		WIN_LEFT_KEYS,
		WIN_RIGHT_KEYS,
		WIN_TOP_KEYS,
		NULL, NULL);
	notify_interpose_event_func(canvas_paint_window(obj),
		(Notify_func) canvas_proc, NOTIFY_SAFE);
	/*
	 * This line is here for backwards compatibility. It will be
	 * removed for the next release.
	 */
	xv_set(canvas_paint_window(obj), XV_KEY_DATA, INSTANCE, ip, NULL);
	return obj;
}

/*
 * Initialize an instance of object `Aframe'.
 */
calctool_Aframe_objects *
calctool_Aframe_objects_initialize(calctool_Aframe_objects *ip, Xv_opaque owner)
{
	if (!ip && !(ip = (calctool_Aframe_objects *) calloc(1, sizeof (calctool_Aframe_objects))))
		return (calctool_Aframe_objects *) NULL;
	if (!ip->Aframe)
		ip->Aframe = calctool_Aframe_Aframe_create(ip, owner);
	if (!ip->Apanel)
		ip->Apanel = aquire_cmd_panel(ip, ip->Aframe);
	if (!ip->Api_text)
		ip->Api_text = calctool_Aframe_Api_text_create(ip, ip->Apanel);
	if (!ip->Adummy)
		ip->Adummy = calctool_Aframe_Adummy_create(ip, ip->Apanel);
	if (!ip->Api_but)
		ip->Api_but = calctool_Aframe_Api_but_create(ip, ip->Apanel);
	if (!ip->Agroup)
		ip->Agroup = calctool_Aframe_Agroup_create(ip, ip->Apanel);
	window_fit(ip->Apanel);
	
	window_fit(ip->Aframe);
	
	xv_set(ip->Apanel,
		XV_WIDTH, WIN_EXTEND_TO_EDGE,
		XV_HEIGHT, WIN_EXTEND_TO_EDGE,
		NULL);
	return ip;
}

/*
 * Create object `Aframe' in the specified instance.
 */
Xv_opaque
calctool_Aframe_Aframe_create(calctool_Aframe_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, FRAME_CMD,
		XV_KEY_DATA, INSTANCE, ip,
		XV_WIDTH, 156,
		XV_HEIGHT, 85,
		XV_LABEL, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Get ASCII"),
		WIN_USE_IM, FALSE,
		XV_SHOW, FALSE,
		FRAME_SHOW_FOOTER, FALSE,
		FRAME_SHOW_RESIZE_CORNER, FALSE,
		FRAME_CMD_PUSHPIN_IN, TRUE,
		NULL);
	return obj;
}


/*
 * Create object `Api_text' in the specified instance.
 */
Xv_opaque
calctool_Aframe_Api_text_create(calctool_Aframe_objects *ip, Xv_opaque owner)
{
	extern Panel_setting	tshow_ascii(Panel_item, Event *);
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TEXT,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 8,
		XV_Y, 8,
		PANEL_VALUE_DISPLAY_LENGTH, 1,
		PANEL_VALUE_STORED_LENGTH, 1,
		PANEL_LABEL_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Character:"),
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_READ_ONLY, FALSE,
		PANEL_NOTIFY_PROC, tshow_ascii,
		NULL);
	return obj;
}

/*
 * Create object `Adummy' in the specified instance.
 */
Xv_opaque
calctool_Aframe_Adummy_create(calctool_Aframe_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 25,
		XV_Y, 36,
		PANEL_LABEL_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "dummy"),
		PANEL_LABEL_BOLD, TRUE,
		XV_SHOW, FALSE,
		NULL);
	return obj;
}

/*
 * Create object `Api_but' in the specified instance.
 */
Xv_opaque
calctool_Aframe_Api_but_create(calctool_Aframe_objects *ip, Xv_opaque owner)
{
	extern void		bshow_ascii(Panel_item, Event *);
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 24,
		XV_Y, 62,
		PANEL_LABEL_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "ASCII"),
		PANEL_NOTIFY_PROC, bshow_ascii,
		NULL);
	return obj;
}

/*
 * Create object `Agroup' in the specified instance.
 */
Xv_opaque
calctool_Aframe_Agroup_create(calctool_Aframe_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, GROUP,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 8,
		XV_Y, 8,
		GROUP_TYPE, GROUP_COLUMN,
		GROUP_MEMBERS,
			ip->Api_text,
			ip->Adummy,
			ip->Api_but,
			NULL,
		GROUP_COLUMN_ALIGNMENT, GROUP_LABELS,
		GROUP_VERTICAL_SPACING, 15,
		NULL);
	return obj;
}

/*
 * Initialize an instance of object `rframe'.
 */
calctool_rframe_objects *
calctool_rframe_objects_initialize(calctool_rframe_objects *ip, Xv_opaque owner)
{
	if (!ip && !(ip = (calctool_rframe_objects *) calloc(1, sizeof (calctool_rframe_objects))))
		return (calctool_rframe_objects *) NULL;
	if (!ip->rframe)
		ip->rframe = calctool_rframe_rframe_create(ip, owner);
	if (!ip->rcanvas)
		ip->rcanvas = calctool_rframe_rcanvas_create(ip, ip->rframe);
	return ip;
}

/*
 * Create object `rframe' in the specified instance.
 */
Xv_opaque
calctool_rframe_rframe_create(calctool_rframe_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, FRAME_CMD,
		XV_KEY_DATA, INSTANCE, ip,
		XV_WIDTH, 248,
		XV_HEIGHT, 179,
		XV_LABEL, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Memory Registers"),
		WIN_USE_IM, FALSE,
		XV_SHOW, FALSE,
		FRAME_SHOW_FOOTER, FALSE,
		FRAME_SHOW_RESIZE_CORNER, FALSE,
		FRAME_CMD_PUSHPIN_IN, TRUE,
		NULL);
	xv_set(xv_get(obj, FRAME_CMD_PANEL), WIN_SHOW, FALSE, NULL);
	return obj;
}

/*
 * Create object `rcanvas' in the specified instance.
 */
Xv_opaque
calctool_rframe_rcanvas_create(calctool_rframe_objects *ip, Xv_opaque owner)
{
	extern void	canvas_repaint(Canvas, Xv_window, Display *, Window, Xv_xrectlist *);
	extern void	canvas_resize(Canvas, int, int);
	Xv_opaque	obj;
	
	obj = xv_create(owner, CANVAS,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 0,
		XV_Y, 0,
		XV_WIDTH, WIN_EXTEND_TO_EDGE,
		XV_HEIGHT, WIN_EXTEND_TO_EDGE,
		CANVAS_REPAINT_PROC, canvas_repaint,
		CANVAS_X_PAINT_WINDOW, TRUE,
		CANVAS_RESIZE_PROC, canvas_resize,
		NULL);
	xv_set(canvas_paint_window(obj), WIN_CONSUME_EVENTS,
		WIN_MOUSE_BUTTONS,
		LOC_MOVE,
		LOC_DRAG,
		LOC_WINENTER,
		LOC_WINEXIT,
		WIN_ASCII_EVENTS,
		WIN_LEFT_KEYS,
		WIN_RIGHT_KEYS,
		WIN_TOP_KEYS,
		NULL, NULL);
	/*
	 * This line is here for backwards compatibility. It will be
	 * removed for the next release.
	 */
	xv_set(canvas_paint_window(obj), XV_KEY_DATA, INSTANCE, ip, NULL);
	return obj;
}

/*
 * Initialize an instance of object `CFframe'.
 */
calctool_CFframe_objects *
calctool_CFframe_objects_initialize(calctool_CFframe_objects *ip, Xv_opaque owner)
{
	if (!ip && !(ip = (calctool_CFframe_objects *) calloc(1, sizeof (calctool_CFframe_objects))))
		return (calctool_CFframe_objects *) NULL;
	if (!ip->CFframe)
		ip->CFframe = calctool_CFframe_CFframe_create(ip, owner);
	if (!ip->CFpanel)
		ip->CFpanel = aquire_cmd_panel(ip, ip->CFframe);
	if (!ip->CFpi_cftext)
		ip->CFpi_cftext = calctool_CFframe_CFpi_cftext_create(ip, ip->CFpanel);
	if (!ip->CFpi_dtext)
		ip->CFpi_dtext = calctool_CFframe_CFpi_dtext_create(ip, ip->CFpanel);
	if (!ip->CFpi_vtext)
		ip->CFpi_vtext = calctool_CFframe_CFpi_vtext_create(ip, ip->CFpanel);
	if (!ip->CFpi_cbut)
		ip->CFpi_cbut = calctool_CFframe_CFpi_cbut_create(ip, ip->CFpanel);
	if (!ip->CFgroup)
		ip->CFgroup = calctool_CFframe_CFgroup_create(ip, ip->CFpanel);
	if (!ip->CFpi_fbut)
		ip->CFpi_fbut = calctool_CFframe_CFpi_fbut_create(ip, ip->CFpanel);
	window_fit(ip->CFpanel);
	
	window_fit(ip->CFframe);
	
	xv_set(ip->CFpanel,
		XV_WIDTH, WIN_EXTEND_TO_EDGE,
		XV_HEIGHT, WIN_EXTEND_TO_EDGE,
		NULL);
	return ip;
}

/*
 * Create object `CFframe' in the specified instance.
 */
Xv_opaque
calctool_CFframe_CFframe_create(calctool_CFframe_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, FRAME_CMD,
		XV_KEY_DATA, INSTANCE, ip,
		XV_WIDTH, 254,
		XV_HEIGHT, 141,
		XV_LABEL, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "New Constant"),
		WIN_USE_IM, TRUE,
		XV_SHOW, FALSE,
		FRAME_SHOW_FOOTER, FALSE,
		FRAME_SHOW_RESIZE_CORNER, FALSE,
		FRAME_CMD_PUSHPIN_IN, TRUE,
		NULL);
	return obj;
}


/*
 * Create object `CFpi_cftext' in the specified instance.
 */
Xv_opaque
calctool_CFframe_CFpi_cftext_create(calctool_CFframe_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TEXT,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 31,
		XV_Y, 8,
		PANEL_VALUE_DISPLAY_LENGTH, 2,
		PANEL_VALUE_STORED_LENGTH, 2,
		PANEL_LABEL_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Constant no:"),
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_READ_ONLY, FALSE,
		NULL);
	return obj;
}

/*
 * Create object `CFpi_dtext' in the specified instance.
 */
Xv_opaque
calctool_CFframe_CFpi_dtext_create(calctool_CFframe_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TEXT,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 33,
		XV_Y, 41,
		PANEL_VALUE_DISPLAY_LENGTH, 22,
		PANEL_VALUE_STORED_LENGTH, 256,
		PANEL_LABEL_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Description:"),
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_READ_ONLY, FALSE,
		NULL);
	return obj;
}

/*
 * Create object `CFpi_vtext' in the specified instance.
 */
Xv_opaque
calctool_CFframe_CFpi_vtext_create(calctool_CFframe_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TEXT,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 67,
		XV_Y, 74,
		PANEL_VALUE_DISPLAY_LENGTH, 22,
		PANEL_VALUE_STORED_LENGTH, 256,
		PANEL_LABEL_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Value:"),
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_READ_ONLY, FALSE,
		NULL);
	return obj;
}

/*
 * Create object `CFpi_cbut' in the specified instance.
 */
Xv_opaque
calctool_CFframe_CFpi_cbut_create(calctool_CFframe_objects *ip, Xv_opaque owner)
{
	extern void		write_cf_value(Panel_item, Event *);
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 12,
		XV_Y, 107,
		PANEL_LABEL_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Enter Constant"),
		PANEL_NOTIFY_PROC, write_cf_value,
		NULL);
	return obj;
}

/*
 * Create object `CFgroup' in the specified instance.
 */
Xv_opaque
calctool_CFframe_CFgroup_create(calctool_CFframe_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, GROUP,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 12,
		XV_Y, 8,
		GROUP_TYPE, GROUP_COLUMN,
		GROUP_MEMBERS,
			ip->CFpi_cftext,
			ip->CFpi_dtext,
			ip->CFpi_vtext,
			ip->CFpi_cbut,
			NULL,
		GROUP_COLUMN_ALIGNMENT, GROUP_LABELS,
		GROUP_VERTICAL_SPACING, 20,
		NULL);
	return obj;
}

/*
 * Create object `CFpi_fbut' in the specified instance.
 */
Xv_opaque
calctool_CFframe_CFpi_fbut_create(calctool_CFframe_objects *ip, Xv_opaque owner)
{
	extern void		write_cf_value(Panel_item, Event *);
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 148,
		XV_Y, 112,
		PANEL_LABEL_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Enter Function"),
		PANEL_NOTIFY_PROC, write_cf_value,
		NULL);
	return obj;
}

/*
 * Initialize an instance of object `Pframe'.
 */
calctool_Pframe_objects *
calctool_Pframe_objects_initialize(calctool_Pframe_objects *ip, Xv_opaque owner)
{
	if (!ip && !(ip = (calctool_Pframe_objects *) calloc(1, sizeof (calctool_Pframe_objects))))
		return (calctool_Pframe_objects *) NULL;
	if (!ip->Pframe)
		ip->Pframe = calctool_Pframe_Pframe_create(ip, owner);
	if (!ip->Ppanel)
		ip->Ppanel = aquire_cmd_panel(ip, ip->Pframe);
	if (!ip->Pappearance)
		ip->Pappearance = calctool_Pframe_Pappearance_create(ip, ip->Ppanel);
	if (!ip->Pdisplay)
		ip->Pdisplay = calctool_Pframe_Pdisplay_create(ip, ip->Ppanel);
	if (!ip->Pstyle)
		ip->Pstyle = calctool_Pframe_Pstyle_create(ip, ip->Ppanel);
	if (!ip->Pgroup2)
		ip->Pgroup2 = calctool_Pframe_Pgroup2_create(ip, ip->Ppanel);
	if (!ip->Papply)
		ip->Papply = calctool_Pframe_Papply_create(ip, ip->Ppanel);
	if (!ip->Pdefaults)
		ip->Pdefaults = calctool_Pframe_Pdefaults_create(ip, ip->Ppanel);
	if (!ip->Preset)
		ip->Preset = calctool_Pframe_Preset_create(ip, ip->Ppanel);
	if (!ip->Pgroup1)
		ip->Pgroup1 = calctool_Pframe_Pgroup1_create(ip, ip->Ppanel);
	if (!ip->Pgroup3)
		ip->Pgroup3 = calctool_Pframe_Pgroup3_create(ip, ip->Ppanel);
	window_fit(ip->Ppanel);
	
	window_fit(ip->Pframe);
	
	xv_set(ip->Ppanel,
		XV_WIDTH, WIN_EXTEND_TO_EDGE,
		XV_HEIGHT, WIN_EXTEND_TO_EDGE,
		NULL);
	return ip;
}

/*
 * Create object `Pframe' in the specified instance.
 */
Xv_opaque
calctool_Pframe_Pframe_create(calctool_Pframe_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, FRAME_CMD,
		XV_KEY_DATA, INSTANCE, ip,
		XV_WIDTH, 241,
		XV_HEIGHT, 171,
		XV_LABEL, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Calculator properties"),
		WIN_USE_IM, FALSE,
		XV_SHOW, FALSE,
		FRAME_SHOW_FOOTER, FALSE,
		FRAME_SHOW_RESIZE_CORNER, FALSE,
		FRAME_CMD_PUSHPIN_IN, TRUE,
		NULL);
	return obj;
}


/*
 * Create object `Pappearance' in the specified instance.
 */
Xv_opaque
calctool_Pframe_Pappearance_create(calctool_Pframe_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_CHOICE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 8,
		XV_Y, 12,
		PANEL_CHOICE_NROWS, 1,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_CHOOSE_NONE, FALSE,
		PANEL_LABEL_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Appearance:"),
		PANEL_CHOICE_STRINGS,
			dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "2D-look"),
			dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "3D-look"),
			NULL,
		NULL);
	return obj;
}

/*
 * Create object `Pdisplay' in the specified instance.
 */
Xv_opaque
calctool_Pframe_Pdisplay_create(calctool_Pframe_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_CHOICE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 32,
		XV_Y, 48,
		PANEL_CHOICE_NROWS, 1,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_CHOOSE_NONE, FALSE,
		PANEL_LABEL_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Display:"),
		PANEL_CHOICE_STRINGS,
			dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "color"),
			dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "monochrome"),
			NULL,
		NULL);
	return obj;
}

/*
 * Create object `Pstyle' in the specified instance.
 */
Xv_opaque
calctool_Pframe_Pstyle_create(calctool_Pframe_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_CHOICE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 44,
		XV_Y, 84,
		PANEL_CHOICE_NROWS, 1,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_CHOOSE_NONE, FALSE,
		PANEL_LABEL_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Style:"),
		PANEL_CHOICE_STRINGS,
			dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "left-handed"),
			dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "right-handed"),
			NULL,
		NULL);
	return obj;
}

/*
 * Create object `Pgroup2' in the specified instance.
 */
Xv_opaque
calctool_Pframe_Pgroup2_create(calctool_Pframe_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, GROUP,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 8,
		XV_Y, 12,
		GROUP_TYPE, GROUP_COLUMN,
		GROUP_MEMBERS,
			ip->Pappearance,
			ip->Pdisplay,
			ip->Pstyle,
			NULL,
		GROUP_COLUMN_ALIGNMENT, GROUP_LABELS,
		GROUP_VERTICAL_SPACING, 15,
		NULL);
	return obj;
}

/*
 * Create object `Papply' in the specified instance.
 */
Xv_opaque
calctool_Pframe_Papply_create(calctool_Pframe_objects *ip, Xv_opaque owner)
{
	extern void		prop_apply(Panel_item, Event *);
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 17,
		XV_Y, 140,
		PANEL_LABEL_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Apply"),
		PANEL_NOTIFY_PROC, prop_apply,
		NULL);
	return obj;
}

/*
 * Create object `Pdefaults' in the specified instance.
 */
Xv_opaque
calctool_Pframe_Pdefaults_create(calctool_Pframe_objects *ip, Xv_opaque owner)
{
	extern void		prop_defaults(Panel_item, Event *);
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 71,
		XV_Y, 140,
		PANEL_LABEL_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Save as Defaults"),
		PANEL_NOTIFY_PROC, prop_defaults,
		NULL);
	return obj;
}

/*
 * Create object `Preset' in the specified instance.
 */
Xv_opaque
calctool_Pframe_Preset_create(calctool_Pframe_objects *ip, Xv_opaque owner)
{
	extern void		prop_reset(Panel_item, Event *);
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 180,
		XV_Y, 140,
		PANEL_LABEL_STRING, dgettext("SUNW_DESKSET_CALCTOOL_LABEL", "Reset"),
		PANEL_NOTIFY_PROC, prop_reset,
		NULL);
	return obj;
}

/*
 * Create object `Pgroup1' in the specified instance.
 */
Xv_opaque
calctool_Pframe_Pgroup1_create(calctool_Pframe_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, GROUP,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 17,
		XV_Y, 140,
		GROUP_TYPE, GROUP_ROW,
		GROUP_MEMBERS,
			ip->Papply,
			ip->Pdefaults,
			ip->Preset,
			NULL,
		GROUP_ROW_ALIGNMENT, GROUP_HORIZONTAL_CENTERS,
		GROUP_HORIZONTAL_SPACING, 10,
		NULL);
	return obj;
}

/*
 * Create object `Pgroup3' in the specified instance.
 */
Xv_opaque
calctool_Pframe_Pgroup3_create(calctool_Pframe_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, GROUP,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 8,
		XV_Y, 12,
		GROUP_TYPE, GROUP_COLUMN,
		GROUP_MEMBERS,
			ip->Pgroup2,
			ip->Pgroup1,
			NULL,
		GROUP_COLUMN_ALIGNMENT, GROUP_VERTICAL_CENTERS,
		GROUP_VERTICAL_SPACING, 35,
		NULL);
	return obj;
}
