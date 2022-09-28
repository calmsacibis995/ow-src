#include "audio_i18n.h"
/*
 * config_ui.c - User interface object initialization functions.
 * This file was generated by `gxv' from `config.G'.
 * DO NOT EDIT BY HAND.
 */

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
#include "config_ui.h"

/*
 * Initialize an instance of object `Popup'.
 */
config_Popup_objects *
config_Popup_objects_initialize(config_Popup_objects *ip, Xv_opaque owner)
{
	if (!ip && !(ip = (config_Popup_objects *) calloc(1, sizeof (config_Popup_objects))))
		return (config_Popup_objects *) NULL;
	if (!ip->Popup)
		ip->Popup = config_Popup_Popup_create(ip, owner);
	if (!ip->Tool_config_control)
		ip->Tool_config_control = config_Popup_Tool_config_control_create(ip, ip->Popup);
	if (!ip->Autoplay_set)
		ip->Autoplay_set = config_Popup_Autoplay_set_create(ip, ip->Tool_config_control);
	if (!ip->AutoSelect_set)
		ip->AutoSelect_set = config_Popup_AutoSelect_set_create(ip, ip->Tool_config_control);
	if (!ip->Confirm_clear_set)
		ip->Confirm_clear_set = config_Popup_Confirm_clear_set_create(ip, ip->Tool_config_control);
	if (!ip->ToggleGroup)
		ip->ToggleGroup = config_Popup_ToggleGroup_create(ip, ip->Tool_config_control);
	if (!ip->SilenceDetect_check)
		ip->SilenceDetect_check = config_Popup_SilenceDetect_check_create(ip, ip->Tool_config_control);
	if (!ip->SilenceDetect_slider)
		ip->SilenceDetect_slider = config_Popup_SilenceDetect_slider_create(ip, ip->Tool_config_control);
	if (!ip->ThresholdGroup)
		ip->ThresholdGroup = config_Popup_ThresholdGroup_create(ip, ip->Tool_config_control);
	if (!ip->tempdir)
		ip->tempdir = config_Popup_tempdir_create(ip, ip->Tool_config_control);
	if (!ip->SettingsGroup)
		ip->SettingsGroup = config_Popup_SettingsGroup_create(ip, ip->Tool_config_control);
	if (!ip->Apply_button)
		ip->Apply_button = config_Popup_Apply_button_create(ip, ip->Tool_config_control);
	if (!ip->Reset_button)
		ip->Reset_button = config_Popup_Reset_button_create(ip, ip->Tool_config_control);
	if (!ip->ApplyGroup)
		ip->ApplyGroup = config_Popup_ApplyGroup_create(ip, ip->Tool_config_control);
	if (!ip->PropsGroup)
		ip->PropsGroup = config_Popup_PropsGroup_create(ip, ip->Tool_config_control);
	window_fit(ip->Tool_config_control);
	
	window_fit(ip->Popup);
	
	xv_set(ip->Tool_config_control,
		XV_WIDTH, WIN_EXTEND_TO_EDGE,
		XV_HEIGHT, WIN_EXTEND_TO_EDGE,
		NULL);
	return ip;
}

/*
 * Create object `Popup' in the specified instance.
 */
Xv_opaque
config_Popup_Popup_create(config_Popup_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, FRAME_CMD,
		XV_KEY_DATA, INSTANCE, ip,
		XV_WIDTH, 301,
		XV_HEIGHT, 257,
		XV_LABEL, dgettext("SUNW_DESKSET_AUDIOTOOL", "Audio Tool: Properties"),
		FRAME_SHOW_FOOTER, TRUE,
		FRAME_SHOW_RESIZE_CORNER, FALSE,
		FRAME_CMD_PUSHPIN_IN, FALSE,
		NULL);
	return obj;
}

/*
 * Create object `Tool_config_control' in the specified instance.
 */
Xv_opaque
config_Popup_Tool_config_control_create(config_Popup_objects *ip, Xv_opaque owner)
{
	Xv_opaque obj = xv_get(owner, FRAME_CMD_PANEL);

	xv_set(obj,
	       XV_KEY_DATA, INSTANCE, ip,
	       XV_HELP_DATA, "audiotool:config-Tool_config_control",
	       NULL);

	return obj;
}

/*
 * Create object `Autoplay_set' in the specified instance.
 */
Xv_opaque
config_Popup_Autoplay_set_create(config_Popup_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TOGGLE, PANEL_FEEDBACK, PANEL_MARKED,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "audiotool:config-Autoplay_set",
		XV_X, 39,
		XV_Y, 8,
		PANEL_CHOICE_NROWS, 1,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_LABEL_STRING, dgettext("SUNW_DESKSET_AUDIOTOOL", "Auto Play on Load:"),
		PANEL_CHOICE_STRING, 0, dgettext("SUNW_DESKSET_AUDIOTOOL", ""),
		PANEL_VALUE, 0,
		NULL);
	return obj;
}

/*
 * Create object `AutoSelect_set' in the specified instance.
 */
Xv_opaque
config_Popup_AutoSelect_set_create(config_Popup_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TOGGLE, PANEL_FEEDBACK, PANEL_MARKED,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "audiotool:config-AutoSelect_set",
		XV_X, 8,
		XV_Y, 41,
		PANEL_CHOICE_NROWS, 1,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_LABEL_STRING, dgettext("SUNW_DESKSET_AUDIOTOOL", "Auto Play on Selection:"),
		PANEL_CHOICE_STRING, 0, dgettext("SUNW_DESKSET_AUDIOTOOL", ""),
		PANEL_VALUE, 0,
		NULL);
	return obj;
}

/*
 * Create object `Confirm_clear_set' in the specified instance.
 */
Xv_opaque
config_Popup_Confirm_clear_set_create(config_Popup_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TOGGLE, PANEL_FEEDBACK, PANEL_MARKED,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "audiotool:config-Confirm_clear_set",
		XV_X, 11,
		XV_Y, 74,
		PANEL_CHOICE_NROWS, 1,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_LABEL_STRING, dgettext("SUNW_DESKSET_AUDIOTOOL", "Confirm on New/Load:"),
		PANEL_CHOICE_STRING, 0, dgettext("SUNW_DESKSET_AUDIOTOOL", ""),
		PANEL_VALUE, 0,
		NULL);
	return obj;
}

/*
 * Create object `ToggleGroup' in the specified instance.
 */
Xv_opaque
config_Popup_ToggleGroup_create(config_Popup_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, GROUP,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 8,
		XV_Y, 8,
		GROUP_TYPE, GROUP_COLUMN,
		GROUP_MEMBERS,
			ip->Autoplay_set,
			ip->AutoSelect_set,
			ip->Confirm_clear_set,
			NULL,
		GROUP_COLUMN_ALIGNMENT, GROUP_LABELS,
		GROUP_VERTICAL_SPACING, 10,
		NULL);
	return obj;
}

/*
 * Create object `SilenceDetect_check' in the specified instance.
 */
Xv_opaque
config_Popup_SilenceDetect_check_create(config_Popup_objects *ip, Xv_opaque owner)
{
	extern void		silence_detect_check_notify(Panel_item, int, Event *);
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TOGGLE, PANEL_FEEDBACK, PANEL_MARKED,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "audiotool:config-SilenceDetect_check",
		XV_X, 41,
		XV_Y, 117,
		PANEL_CHOICE_NCOLS, 1,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_LABEL_STRING, dgettext("SUNW_DESKSET_AUDIOTOOL", "Silence Detection:"),
		PANEL_NOTIFY_PROC, silence_detect_check_notify,
		PANEL_CHOICE_STRING, 0, dgettext("SUNW_DESKSET_AUDIOTOOL", ""),
		PANEL_VALUE, 0,
		NULL);
	return obj;
}

/*
 * Create object `SilenceDetect_slider' in the specified instance.
 */
Xv_opaque
config_Popup_SilenceDetect_slider_create(config_Popup_objects *ip, Xv_opaque owner)
{
	extern void		SilenceDetect_slider_notify(Panel_item, int, Event *);
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_SLIDER,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "audiotool:config-SilenceDetect_slider",
		XV_X, 36,
		XV_Y, 150,
		PANEL_SLIDER_WIDTH, 100,
		PANEL_TICKS, 17,
		PANEL_LABEL_STRING, dgettext("SUNW_DESKSET_AUDIOTOOL", "Silence Threshold:"),
		PANEL_DIRECTION, PANEL_HORIZONTAL,
		PANEL_SLIDER_END_BOXES, FALSE,
		PANEL_SHOW_RANGE, FALSE,
		PANEL_SHOW_VALUE, FALSE,
		PANEL_MIN_VALUE, -8,
		PANEL_MAX_VALUE, 8,
		PANEL_VALUE, 0,
		PANEL_NOTIFY_PROC, SilenceDetect_slider_notify,
		NULL);
	return obj;
}

/*
 * Create object `ThresholdGroup' in the specified instance.
 */
Xv_opaque
config_Popup_ThresholdGroup_create(config_Popup_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, GROUP,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 36,
		XV_Y, 117,
		GROUP_TYPE, GROUP_COLUMN,
		GROUP_MEMBERS,
			ip->SilenceDetect_check,
			ip->SilenceDetect_slider,
			NULL,
		GROUP_COLUMN_ALIGNMENT, GROUP_LABELS,
		GROUP_VERTICAL_SPACING, 10,
		NULL);
	return obj;
}

/*
 * Create object `tempdir' in the specified instance.
 */
Xv_opaque
config_Popup_tempdir_create(config_Popup_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TEXT,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "audiotool:config-tempdir",
		XV_X, 28,
		XV_Y, 196,
		PANEL_VALUE_DISPLAY_LENGTH, 15,
		PANEL_VALUE_STORED_LENGTH, 255,
		PANEL_LABEL_STRING, dgettext("SUNW_DESKSET_AUDIOTOOL", "Temp file directory:"),
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_VALUE, dgettext("SUNW_DESKSET_AUDIOTOOL", "/tmp"),
		PANEL_READ_ONLY, FALSE,
		NULL);
	return obj;
}

/*
 * Create object `SettingsGroup' in the specified instance.
 */
Xv_opaque
config_Popup_SettingsGroup_create(config_Popup_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, GROUP,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 8,
		XV_Y, 8,
		GROUP_TYPE, GROUP_COLUMN,
		GROUP_MEMBERS,
			ip->ToggleGroup,
			ip->ThresholdGroup,
			ip->tempdir,
			NULL,
		GROUP_COLUMN_ALIGNMENT, GROUP_LABELS,
		GROUP_VERTICAL_SPACING, 20,
		NULL);
	return obj;
}

/*
 * Create object `Apply_button' in the specified instance.
 */
Xv_opaque
config_Popup_Apply_button_create(config_Popup_objects *ip, Xv_opaque owner)
{
	extern void		apply_button_notify(Panel_item, Event *);
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "audiotool:config-Apply_button",
		XV_X, 87,
		XV_Y, 231,
		PANEL_LABEL_STRING, dgettext("SUNW_DESKSET_AUDIOTOOL", "Apply"),
		PANEL_NOTIFY_PROC, apply_button_notify,
		NULL);
	return obj;
}

/*
 * Create object `Reset_button' in the specified instance.
 */
Xv_opaque
config_Popup_Reset_button_create(config_Popup_objects *ip, Xv_opaque owner)
{
	extern void		Reset_button_notify(Panel_item, Event *);
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
		XV_KEY_DATA, INSTANCE, ip,
		XV_HELP_DATA, "audiotool:config-Reset_button",
		XV_X, 160,
		XV_Y, 231,
		PANEL_LABEL_STRING, dgettext("SUNW_DESKSET_AUDIOTOOL", "Reset"),
		PANEL_NOTIFY_PROC, Reset_button_notify,
		NULL);
	return obj;
}

/*
 * Create object `ApplyGroup' in the specified instance.
 */
Xv_opaque
config_Popup_ApplyGroup_create(config_Popup_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, GROUP,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 87,
		XV_Y, 231,
		GROUP_TYPE, GROUP_ROW,
		GROUP_MEMBERS,
			ip->Apply_button,
			ip->Reset_button,
			NULL,
		GROUP_ROW_ALIGNMENT, GROUP_HORIZONTAL_CENTERS,
		GROUP_HORIZONTAL_SPACING, 20,
		NULL);
	return obj;
}

/*
 * Create object `PropsGroup' in the specified instance.
 */
Xv_opaque
config_Popup_PropsGroup_create(config_Popup_objects *ip, Xv_opaque owner)
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, GROUP,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 8,
		XV_Y, 8,
		GROUP_TYPE, GROUP_COLUMN,
		GROUP_MEMBERS,
			ip->SettingsGroup,
			ip->ApplyGroup,
			NULL,
		GROUP_COLUMN_ALIGNMENT, GROUP_VERTICAL_CENTERS,
		GROUP_VERTICAL_SPACING, 20,
		NULL);
	return obj;
}

