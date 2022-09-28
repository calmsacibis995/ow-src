/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)config.c	1.31	93/02/18 SMI"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <math.h>

#include "atool_i18n.h"

#include "config_panel_impl.h"
#include "config_panel.h"
#include "silent_params.h"

#include "defaults.h"		/* defaults package */

#ifndef TRUE
#define TRUE            1
#endif /*!TRUE*/

#ifndef FALSE
#define FALSE            0
#endif /*!FALSE*/

static struct defaults_list audio_defaults[] =
{
    /* audiotool config prop's */

    "autoPlayOnLoad",		"AutoPlayOnLoad", DT_BOOL, (ptr_t) TRUE,
    "Start playing audio file after load",
    NULL, TRUE,

    "autoPlayOnSel",		"AutoPlayOnSel", DT_BOOL,  (ptr_t) FALSE,
    "Start playing after a selection is made",
    NULL, TRUE, 

    "confirmClear",		"ConfirmClear",	DT_BOOL, (ptr_t) TRUE,
    "Confirm before clearing edit buffer",
    NULL, TRUE, 

    "silenceDetection",		"SilenceDetection",	DT_BOOL, (ptr_t) TRUE,
    "Enable/Disable Silence Detection Graph",
    NULL, TRUE,

    "silenceThreshold",		"SilenceThreshold",	DT_INT, (ptr_t) 0,
    "Silence Detection Threshold",
    NULL, TRUE,

    "tempDirectory",		"TempDirectory",       DT_STR, (ptr_t) "/tmp",
    "Path to directory for record temp file storage",
    NULL, TRUE,

    /* list of custom file formats */
    "formatList",		"FormatList",		DT_STR, (ptr_t) NULL,
    NULL, FALSE,

    NULL, NULL, 0, 0, NULL, NULL,
};


/*
 * Create a Config Data Panel
 */
/* XXX - need to remove the obsolete stuff from here ... */
ptr_t
ConfigPanel_Init(ptr_t owner,
		 void (*set_autoplay_proc) (/*XXX*/),
		 void (*set_autosel_proc) (/*XXX*/),
		 void (*set_confirm_proc) (/*XXX*/),
		 void (*set_silence_proc) (/*XXX*/),
		 void (*set_threshold_proc) (/*XXX*/),
		 void (*set_tempdir_proc) (/*XXX*/),
		 void (*apply_proc) (/*XXX*/))
{
	struct config_panel_data	*dp;

	/* allocate storage for file panel data */
	if ((dp = (struct config_panel_data *)calloc(1, sizeof (*dp))) == NULL)
		return (NULL);
	dp->owner = owner;

	/* init call backs */
	dp->set_silence_proc = set_silence_proc;
	dp->set_threshold_proc = set_threshold_proc;
	dp->set_autoplay_proc = set_autoplay_proc;
	dp->set_autosel_proc = set_autosel_proc;
	dp->set_confirm_proc = set_confirm_proc;
	dp->set_tempdir_proc = set_tempdir_proc;
	dp->apply_proc = apply_proc;

	/* init the settings */
	if ((dp->settings = (struct config_settings *) 
	     calloc(1, sizeof (struct config_settings))) == NULL)
	    return (NULL);

	/* Init the file panel window */
	dp->panel = ConfigPanel_INIT(owner, (ptr_t)dp);

	if (dp->panel == NULL) {
		(void) free((ptr_t)dp);
		return (NULL);
	}

	/* initialize the defaults structure
	 * XXX this stuff *really* belongs in the _xv.c file ...
	 */
	if (dp->ddp = InitDefaults(audio_defaults, 
				   DEFAULTS_APP_NAME, 
				   DEFAULTS_APP_CLASS, 
				   DEFAULTS_APP_FILE, dp)) {
		SetDefaultCallback(dp->ddp, "autoPlayOnLoad",
				   set_autoplay_proc);
		SetDefaultCallback(dp->ddp, "autoPlayOnSel",
				   set_autosel_proc);
		SetDefaultCallback(dp->ddp, "confirmClear",
				   set_confirm_proc);
		SetDefaultCallback(dp->ddp, "silenceDetection",
				   set_silence_proc);
		SetDefaultCallback(dp->ddp, "tempDirectory",
				   set_tempdir_proc);
#ifdef notdef
		/* have to do this from apply proc ... */
		SetDefaultCallback(dp->ddp, "silenceTreshold",
				   set_threshold_proc);
#endif


	}

	return ((ptr_t) dp);
}

int
ConfigPanel_SetDefaults(ptr_t idp)
{
	struct config_panel_data	*dp;
	int thresh;

	dp = (struct config_panel_data *) idp;

	/* this will fire all the callback's! */
	ReadDefaults(dp->ddp);

	/* now set all the panel items to reflect reality */

	ConfigPanel_SetAutosel(dp, (int)GetDefault(dp->ddp, "autoPlayOnSel"));
	ConfigPanel_SetAutoplay(dp, (int)GetDefault(dp->ddp, "autoPlayOnLoad"));
	ConfigPanel_SetConfirm(dp, (int)GetDefault(dp->ddp, "confirmClear"));
	ConfigPanel_SetSilence(dp, (int)GetDefault(dp->ddp, "silenceDetection"));
	ConfigPanel_SetThreshold(dp,
		thresh = (int)GetDefault(dp->ddp, "silenceThreshold"));
	ConfigPanel_SetTempdir(dp,
		(char *)GetDefault(dp->ddp, "tempDirectory"));

	/* XXX - need to explicitly fire callback for threshold ... */
	(*dp->set_threshold_proc)(dp,
				  min_silence(thresh),
				  min_sound(thresh),
				  threshold_scale(thresh),
				  noise_ratio(thresh));

}

/* Set the left lower footer of the panel */
void
ConfigPanel_Setfooter(ptr_t idp, char *str)
{
	struct config_panel_data	*dp;

	dp = (struct config_panel_data *)idp;
	ConfigPanel_SETLEFTFOOTER(dp->panel, str);
}

/* Return the File Data Panel owner handle */
ptr_t
ConfigPanel_Getowner(ptr_t idp)
{
	struct config_panel_data	*dp;

	dp = (struct config_panel_data *)idp;
	return ((ptr_t)dp->owner);
}

/* Return the File Data Panel window handle */
ptr_t
ConfigPanel_Gethandle(ptr_t idp)
{
	struct config_panel_data	*dp;

	dp = (struct config_panel_data *)idp;
	return ((ptr_t)dp->panel);
}
/* Pop up the File Data Panel */
void
ConfigPanel_Show(ptr_t idp)
{
	struct config_panel_data	*dp;

	dp = (struct config_panel_data *)idp;

	/* Make sure the panel info is current */
	ConfigPanel_SHOW(dp->panel);
}

/* Dismiss the File Data Panel */
void
ConfigPanel_Unshow(ptr_t idp)
{
	struct config_panel_data	*dp;

	dp = (struct config_panel_data *)idp;
	ConfigPanel_UNSHOW(dp->panel);
}

int
ConfigPanel_SetAutoplay(ptr_t idp, int value)
{
	struct config_panel_data 	*dp;

	dp = (struct config_panel_data *) idp;
	dp->settings->autoplay = value;
	ConfigPanel_SETAUTOPLAY(dp->panel, value);
}

int
ConfigPanel_SetAutosel(ptr_t idp, int value)
{
	struct config_panel_data 	*dp;

	dp = (struct config_panel_data *) idp;
	dp->settings->autosel = value;
	ConfigPanel_SETAUTOSEL(dp->panel, value);
}

int
ConfigPanel_SetConfirm(ptr_t idp, int value)
{
	struct config_panel_data 	*dp;

	dp = (struct config_panel_data *) idp;
	dp->settings->confirm = value;
	ConfigPanel_SETCONFIRM(dp->panel, value);
}

int
ConfigPanel_SetSilence(ptr_t idp, int value)
{
	struct config_panel_data 	*dp;

	dp = (struct config_panel_data *) idp;
	dp->settings->silence = value;
	ConfigPanel_SETSILENCE(dp->panel, value);
}

int
ConfigPanel_SetThreshold(ptr_t idp, int value)
{
	struct config_panel_data 	*dp;

	dp = (struct config_panel_data *) idp;
	dp->settings->threshold = value;
	ConfigPanel_SETTHRESHOLD(dp->panel, value);
}

int
ConfigPanel_SetTempdir(ptr_t idp, char *value)
{
	struct config_panel_data 	*dp;

	dp = (struct config_panel_data *) idp;
	if (dp->settings->tempdir != NULL)
		(void) free(dp->settings->tempdir);

	/* Duplicate the string and set it as the new default */
	dp->settings->tempdir = strdup(value);
	(void) SetDefault(dp->ddp, "tempDirectory",
	    (ptr_t) dp->settings->tempdir);

	ConfigPanel_SETTEMPDIR(dp->panel, dp->settings->tempdir);
}

int
ConfigPanel_GetAutoplay(ptr_t idp)
{
	struct config_panel_data 	*dp;

	dp = (struct config_panel_data *) idp;
	return (dp->settings->autoplay = ConfigPanel_GETAUTOPLAY(dp->panel));
}

int
ConfigPanel_GetAutosel(ptr_t idp)
{
	struct config_panel_data 	*dp;

	dp = (struct config_panel_data *) idp;
	return (dp->settings->autosel = ConfigPanel_GETAUTOSEL(dp->panel));
}

int
ConfigPanel_GetConfirm(ptr_t idp)
{
	struct config_panel_data 	*dp;

	dp = (struct config_panel_data *) idp;
	return (dp->settings->confirm = ConfigPanel_GETCONFIRM(dp->panel));
}

int
ConfigPanel_GetSilence(ptr_t idp)
{
	struct config_panel_data 	*dp;

	dp = (struct config_panel_data *) idp;
	return (dp->settings->silence = ConfigPanel_GETSILENCE(dp->panel));
}

int
ConfigPanel_GetThreshold(ptr_t idp)
{
	struct config_panel_data 	*dp;

	dp = (struct config_panel_data *) idp;
	return (dp->settings->threshold = ConfigPanel_GETTHRESHOLD(dp->panel));
}

char *
ConfigPanel_GetTempdir(ptr_t idp)
{
	struct config_panel_data 	*dp;

	dp = (struct config_panel_data *) idp;
	if (dp->settings->tempdir != NULL)
		(void) free(dp->settings->tempdir);
	return (dp->settings->tempdir = strdup(
		(char*)ConfigPanel_GETTEMPDIR(dp->panel)));
}

ConfigPanel_Setdefault(ptr_t idp, char *name, ptr_t val)
{
	struct config_panel_data	*dp;

	dp = (struct config_panel_data *) idp;
	SetDefault(dp->ddp, name, val);
}

ptr_t
ConfigPanel_Getdefault(ptr_t idp, char *name)
{
	struct config_panel_data	*dp;

	dp = (struct config_panel_data *) idp;
	return(GetDefault(dp->ddp, name));
}

ConfigPanel_Writedefaults(ptr_t idp)
{
	struct config_panel_data	*dp;

	dp = (struct config_panel_data *) idp;
	WriteDefaults(dp->ddp);
}
