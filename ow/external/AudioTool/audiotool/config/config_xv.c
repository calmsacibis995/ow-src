/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)config_xv.c	1.33	93/02/18 SMI"

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <math.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/xv_xrect.h>
#include <gdd.h>

#include "config_ui.h"

#include "atool_i18n.h"
#include "atool_debug.h"
#include "ds_popup.h"		/* deskset popup routines */

#include "config_panel_impl.h"
#include "config_panel.h"

#include "silent_params.h"

Attr_attribute INSTANCE;	/* devguide global key ... */
Attr_attribute CONFIGKEY;	/* do i need this? */

Attr_attribute FPSLIDERKEY;	/* for sliders - to cvt to floating pt. */

#ifdef MAIN

/*
 * Instance XV_KEY_DATA key.  An instance is a set of related
 * user interface objects.  A pointer to an object's instance
 * is stored under this key in every object.  This must be a
 * global variable.
 */
Attr_attribute	INSTANCE;

void
main(argc, argv)
	int		argc;
	char		**argv;
{
	config_Popup_objects	*config_popup;

	/*
	 * Initialize XView.
	 */
	xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, 
		0);
	INSTANCE = xv_unique_key();
	
	/*
	 * Initialize user interface components.
	 */
	config_popup = config_Popup_objects_initialize(NULL, NULL);
	
	/*
	 * Turn control over to XView.
	 */
	xv_main_loop(config_popup->Config_popup);
	exit(0);
}

#endif
/* Create a Config Panel */
ptr_t
ConfigPanel_INIT(ptr_t owner, ptr_t dp)
{
	config_Popup_objects		*ip;

	/* Init global keys, if necessary */
	if (INSTANCE == 0)
		INSTANCE = xv_unique_key();
	if (CONFIGKEY == 0)
	    	CONFIGKEY = xv_unique_key();

	/* Initialize XView status panel */
	if ((ip = config_Popup_objects_initialize(NULL, (Xv_opaque)owner)) == NULL)
		return (NULL);

	/* Save the address of local storage */
	xv_set(ip->CONFIG_PANEL, XV_KEY_DATA, CONFIGKEY, dp, NULL);

	/* init silence detection function domain to slider range */
	sdf_init(
	    (int) xv_get(ip->SILENCE_DETECT_SLIDER, PANEL_MIN_VALUE),
	    (int) xv_get(ip->SILENCE_DETECT_SLIDER, PANEL_MAX_VALUE));


	return ((ptr_t) ip->CONFIG_PANEL);
}

/* Set the File Panel Label */
void
ConfigPanel_SETLABEL(ptr_t cp, char *str)
	    		     
{
	xv_set((Xv_opaque)cp, FRAME_LABEL, str, NULL);
}

/* set left footer label */
void
ConfigPanel_SETLEFTFOOTER(ptr_t cp, char *str)
	       		   	/* config panel */
	    		     
{
	xv_set((Xv_opaque)cp, FRAME_LEFT_FOOTER, str, NULL);
}

/* Pop up the File Panel */
void
ConfigPanel_SHOW(ptr_t cp)
{
	config_Popup_objects		*ip;
	struct config_panel_data	*dp;
	static int 			mapped = FALSE;

	ip = (config_Popup_objects *) xv_get((Xv_opaque)cp,
					     XV_KEY_DATA, INSTANCE);
	dp = (struct config_panel_data *) 
	    xv_get(ip->CONFIG_PANEL, XV_KEY_DATA, CONFIGKEY);
	
	/* position it */
	if (dp->owner && (mapped == FALSE)) {
		ds_position_popup((Xv_opaque)dp->owner, (Xv_opaque)dp->panel,
		    DS_POPUP_LOR);
		mapped = TRUE;
	}

	xv_set((Xv_opaque)cp, XV_SHOW, TRUE, NULL);
}

/* Dismiss the File Panel */
void
ConfigPanel_UNSHOW(ptr_t cp)
	       			   	/* config panel */
{
	xv_set((Xv_opaque)cp, FRAME_CMD_PUSHPIN_IN, FALSE, XV_SHOW, FALSE, 0);
}

void
ConfigPanel_SETAUTOPLAY(ptr_t cp, int value)
{
	config_Popup_objects		*ip;

	ip = (config_Popup_objects *) xv_get((Xv_opaque)cp, XV_KEY_DATA, INSTANCE);

	xv_set(ip->AUTOPLAY_SET, PANEL_VALUE, value ? 1 : 0, NULL);
}

void
ConfigPanel_SETAUTOSEL(ptr_t cp, int value)
{
	config_Popup_objects		*ip;

	ip = (config_Popup_objects *) xv_get((Xv_opaque)cp, XV_KEY_DATA, INSTANCE);

	xv_set(ip->AUTOSELECT_SET, PANEL_VALUE, value ? 1 : 0, NULL);
}

void
ConfigPanel_SETCONFIRM(ptr_t cp, int value)
{
	config_Popup_objects		*ip;

	ip = (config_Popup_objects *) xv_get((Xv_opaque)cp, XV_KEY_DATA, INSTANCE);

	xv_set(ip->CONFIRM_CLEAR_SET, PANEL_VALUE, value ? 1 : 0, NULL);
}

void
ConfigPanel_SETSILENCE(ptr_t cp, int value)
{
	config_Popup_objects		*ip;

	ip = (config_Popup_objects *) xv_get((Xv_opaque)cp, XV_KEY_DATA, INSTANCE);

	xv_set(ip->SILENCE_DETECT_SWITCH, PANEL_VALUE, value ? 1 : 0, NULL);
	xv_set(ip->SILENCE_DETECT_SLIDER, PANEL_INACTIVE, (value == 0), NULL);
}

void
ConfigPanel_SETTHRESHOLD(ptr_t cp, int value)
{
	config_Popup_objects		*ip;

	ip = (config_Popup_objects *) xv_get((Xv_opaque)cp, XV_KEY_DATA, INSTANCE);

	xv_set(ip->SILENCE_DETECT_SLIDER, PANEL_VALUE, value, NULL);
}

void
ConfigPanel_SETTEMPDIR(ptr_t cp, char *value)
{
	config_Popup_objects		*ip;

	ip = (config_Popup_objects *) xv_get((Xv_opaque)cp, XV_KEY_DATA, INSTANCE);

	xv_set(ip->TEMP_DIR_TEXT, PANEL_VALUE, value, NULL);
}

int
ConfigPanel_GETAUTOPLAY(ptr_t cp)
{
	config_Popup_objects		*ip;

	ip = (config_Popup_objects *) xv_get((Xv_opaque)cp, XV_KEY_DATA, INSTANCE);

	/* 0'th index == "YES" */
	return (int)xv_get(ip->AUTOPLAY_SET, PANEL_VALUE);
}

int
ConfigPanel_GETAUTOSEL(ptr_t cp)
{
	config_Popup_objects		*ip;

	ip = (config_Popup_objects *) xv_get((Xv_opaque)cp, XV_KEY_DATA, INSTANCE);

	/* 0'th index == "YES" */
	return (int)xv_get(ip->AUTOSELECT_SET, PANEL_VALUE);
}

int
ConfigPanel_GETCONFIRM(ptr_t cp)
{
	config_Popup_objects		*ip;

	ip = (config_Popup_objects *) xv_get((Xv_opaque)cp, XV_KEY_DATA, INSTANCE);

	return (int)xv_get(ip->CONFIRM_CLEAR_SET, PANEL_VALUE);
}

int
ConfigPanel_GETSILENCE(ptr_t cp)
{
	config_Popup_objects		*ip;

	ip = (config_Popup_objects *) xv_get((Xv_opaque)cp, XV_KEY_DATA, INSTANCE);

	return (int)xv_get(ip->SILENCE_DETECT_SWITCH, PANEL_VALUE);
}

int
ConfigPanel_GETTHRESHOLD(ptr_t cp)
{
	config_Popup_objects		*ip;

	ip = (config_Popup_objects *) xv_get((Xv_opaque)cp, XV_KEY_DATA, INSTANCE);

	return (int)xv_get(ip->SILENCE_DETECT_SLIDER, PANEL_VALUE);
}

char *
ConfigPanel_GETTEMPDIR(ptr_t cp)
{
	config_Popup_objects		*ip;

	ip = (config_Popup_objects *) xv_get((Xv_opaque)cp, XV_KEY_DATA, INSTANCE);

	return (char *)xv_get(ip->TEMP_DIR_TEXT, PANEL_VALUE);
}

/*
 * Notify callback function for `SilenceDetect_slider'.
 */
void
SilenceDetect_slider_notify(Panel_item item, int value, Event *event)
{
	config_Popup_objects *ip = (config_Popup_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	DBGOUT((1, "SilenceDetect_slider_notify: value: %d\n", value));
	sdf_params(value);
}


/*
 * Notify callback function for `SilenceDetect_check'.
 */
void
silence_detect_check_notify(Panel_item item, int value, Event *event)
{
	config_Popup_objects *ip = (config_Popup_objects *) 
		xv_get(item, XV_KEY_DATA, INSTANCE);
	
	xv_set(ip->SilenceDetect_slider, PANEL_INACTIVE, (value == 0), NULL);
}
/*
 * Notify callback function for `Autoplay_set'.
 */
void
autoplay_set_notify(Panel_item item, int value, Event *event)
{
	config_Popup_objects	*ip = (config_Popup_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
}

/*
 * Notify callback function for `AutoSelect_set'.
 */
void
autoselect_set_notify(Panel_item item, int value, Event *event)
{
	config_Popup_objects	*ip = (config_Popup_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
}

/*
 * Notify callback function for `Apply_button'.
 */
void
apply_button_notify(Panel_item item, Event *event)
{
	config_Popup_objects		*ip;
	struct config_panel_data	*dp;
	struct config_settings		savesettings;
	/* set if any of the defaults have changed */
	int				changeflag = 0;
	int				threshold;

	ip = (config_Popup_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	dp = (struct config_panel_data *) 
	    xv_get(ip->CONFIG_PANEL, XV_KEY_DATA, CONFIGKEY);
	
	/* first copy the old settings needed later */
	savesettings.silence = dp->settings->silence;
	savesettings.threshold = dp->settings->threshold;

	/*
	 * Do a get on all the objects (read their values from the panel),
	 * invoke the callbacks for each item with that value,
	 * invoke the apply proc.  This will fire all the callbacks.
	 */
	changeflag += SetDefault(dp->ddp, "autoPlayOnLoad", 
				 (ptr_t)ConfigPanel_GetAutoplay(dp));
	changeflag += SetDefault(dp->ddp, "autoPlayOnSel", 
				 (ptr_t)ConfigPanel_GetAutosel(dp));
	changeflag += SetDefault(dp->ddp, "confirmClear", 
				 (ptr_t)ConfigPanel_GetConfirm(dp));

	changeflag += SetDefault(dp->ddp, "silenceDetection",
				 (ptr_t)ConfigPanel_GetSilence(dp));
	changeflag += SetDefault(dp->ddp, "silenceThreshold", 
				 (ptr_t)ConfigPanel_GetThreshold(dp));
	changeflag += SetDefault(dp->ddp, "tempDirectory", 
				 (ptr_t)ConfigPanel_GetTempdir(dp));

	/* now run the callback for the threshold settings, converting
	 * the slider value to the actual threshold settings
	 */
	threshold = (int) GetDefault(dp->ddp, "silenceThreshold");
	(*dp->set_threshold_proc)(dp,
				  min_silence(threshold),
				  min_sound(threshold),
				  threshold_scale(threshold),
				  noise_ratio(threshold));

	/* only write to .audiorc if any of them changed */
	if (changeflag >= 1) {
		ConfigPanel_Setfooter(dp, MGET("Writing defaults"));
		xv_set((Xv_opaque)dp->panel, FRAME_BUSY, TRUE, 0);

		WriteDefaults(dp->ddp);

		xv_set((Xv_opaque)dp->panel, FRAME_BUSY, FALSE, 0);
		ConfigPanel_Setfooter(dp, MGET("Done"));
	}

	/* Don't apply (update graph) unless threshold settings changed */
	if ((savesettings.silence != dp->settings->silence) ||
	    (dp->settings->silence &&
	     (savesettings.threshold != dp->settings->threshold)))
		(*dp->apply_proc)(dp);	/* update graph ... */

	ConfigPanel_Setfooter(dp, "");
}

/*
 * Notify callback function for `Reset_button'.
 */
void
Reset_button_notify(Panel_item item, Event *event)
{
	config_Popup_objects		*ip;
	struct config_panel_data	*dp;

	ip = (config_Popup_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	dp = (struct config_panel_data *) 
	    xv_get(ip->CONFIG_PANEL, XV_KEY_DATA, CONFIGKEY);
	
	/* Reset the panel items to the values in the settings struct */

	ConfigPanel_SETAUTOPLAY(dp->panel, dp->settings->autoplay);
	ConfigPanel_SETAUTOSEL(dp->panel, dp->settings->autosel);
	ConfigPanel_SETCONFIRM(dp->panel, dp->settings->confirm);
	ConfigPanel_SETSILENCE(dp->panel, dp->settings->silence);
	ConfigPanel_SETTHRESHOLD(dp->panel, dp->settings->threshold);
	ConfigPanel_SETTEMPDIR(dp->panel, dp->settings->tempdir);
}
