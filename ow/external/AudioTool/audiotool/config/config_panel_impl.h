/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _AUDIOTOOL_CONFIG_PANEL_IMPL_H
#define	_AUDIOTOOL_CONFIG_PANEL_IMPL_H

#ident	"@(#)config_panel_impl.h	1.17	92/12/14 SMI"

#include <atool_types.h>
#include "config_panel.h"
#include "defaults.h"

/* map panel item names in case devguide names change */

#define CONFIG_PANEL			Popup

#define SILENCE_DETECT_GROUP		SilenceDetectGroup
#define SILENCE_DETECT_SWITCH		SilenceDetect_check
#define SILENCE_DETECT_SLIDER		SilenceDetect_slider
#define SILENCE_DETECT_LEFTLABEL	SilenceDetect_leftmsg
#define SILENCE_DETECT_RIGHTLABEL	SilenceDetect_rightmsg

#define AUTOPLAY_SET			Autoplay_set
#define AUTOSELECT_SET			AutoSelect_set
#define CONFIRM_CLEAR_SET		Confirm_clear_set
#define APPLY_BUTTON			Apply_button
#define RESET_BUTTON			Reset_button
#define TEMP_DIR_TEXT			tempdir

/* min's, max's and scaling values to map slider values (int's) to
 * actual threshold settings (doubles).
 */

#define SILENCE_SCALE			0.005
#define SOUND_SCALE			0.01
#define THRESHOLD_SCALE			0.04
#define NOISE_SCALE			0.01
#define GRAPH_SCALE			10.0

/* used to convert threshold values (usually double's) to graphical 
 * representation value (usually an int), and vice versa....
 */

#define CVTODBL(val, scale) ((double)(val * (double)scale))
#define CVTOGRAPH(val, scale) (nint(val / (double)scale)) /* needs math.h */

/* config settings */

struct config_settings {
	int			autoplay;
	int			autosel;
	int			confirm;
	int			silence;
	int			threshold;
	char			*tempdir;
};

/* panel call backs and other data  */

struct config_panel_data {
	ptr_t                 owner;	/* handle to owner of config panel */
	ptr_t                 panel;	/* handle to config panel */

	/* ptr to config settings */
	struct config_settings	*settings;

	/* call backs for setting various config attr's */
	void			(*set_silence_proc)();
	void			(*set_threshold_proc)();
	void			(*set_autoplay_proc)();
	void			(*set_autosel_proc)();
	void			(*set_confirm_proc)();
	void			(*set_tempdir_proc)();
	void			(*apply_proc)();

	struct defaults_data	*ddp;
};

/* Define private interfaces */
extern ptr_t  ConfigPanel_INIT(ptr_t owner, ptr_t dp);
extern void     ConfigPanel_SHOW(ptr_t cp);
extern void     ConfigPanel_UNSHOW(ptr_t cp);

extern void     ConfigPanel_SETLEFTFOOTER(ptr_t cp, char *str);
extern void     ConfigPanel_SETLABEL(ptr_t cp, char *str);
	
extern void	ConfigPanel_SETSILENCE(ptr_t cp, int value);
extern void	ConfigPanel_SETTHRESHOLD(ptr_t cp, int value);
extern void	ConfigPanel_SETAUTOPLAY(ptr_t cp, int value);
extern void	ConfigPanel_SETAUTOSEL(ptr_t cp, int value);
extern void	ConfigPanel_SETCONFIRM(ptr_t cp, int value);
extern void	ConfigPanel_SETTEMPDIR(ptr_t cp, char *value);

extern int	ConfigPanel_GETSILENCE(ptr_t cp);
extern int	ConfigPanel_GETTHRESHOLD(ptr_t cp);
extern int	ConfigPanel_GETAUTOPLAY(ptr_t cp);
extern int	ConfigPanel_GETAUTOSEL(ptr_t cp);
extern int	ConfigPanel_GETCONFIRM(ptr_t cp);
extern char 	*ConfigPanel_GETTEMPDIR(ptr_t cp);

#endif /* !_AUDIOTOOL_CONFIG_PANEL_IMPL_H */

