/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _AUDIOTOOL_CONFIG_PANEL_H
#define	_AUDIOTOOL_CONFIG_PANEL_H

#ident	"@(#)config_panel.h	1.11	92/12/14 SMI"

#include "atool_types.h"

/* Define public interfaces */
extern ptr_t ConfigPanel_Init(ptr_t owner,
				PFUNCV set_autoplay_proc,
				PFUNCV set_autosel_proc,
				PFUNCV set_confirm_proc,
				PFUNCV set_silence_proc,
				PFUNCV set_threshold_proc,
				PFUNCV set_tempdir_proc,
				PFUNCV apply_proc
				);
extern ptr_t	ConfigPanel_Gethandle(ptr_t idp);
extern ptr_t	ConfigPanel_Getowner(ptr_t idp);
extern void	ConfigPanel_Show(ptr_t idp);
extern void	ConfigPanel_Unshow(ptr_t idp);

extern int	ConfigPanel_SetSilence(ptr_t idp, int value);
extern int	ConfigPanel_SetThreshold(ptr_t idp, int value);
extern int	ConfigPanel_SetAutoplay(ptr_t idp, int value);
extern int	ConfigPanel_SetAutosel(ptr_t idp, int value);
extern int	ConfigPanel_SetConfirm(ptr_t idp, int value);
extern int	ConfigPanel_SetTempdir(ptr_t idp, char *value);

extern int	ConfigPanel_GetSilence(ptr_t idp);
extern int	ConfigPanel_GetThreshold(ptr_t idp);
extern int	ConfigPanel_GetAutoplay(ptr_t idp);
extern int	ConfigPanel_GetAutosel(ptr_t idp);
extern int	ConfigPanel_GetConfirm(ptr_t idp);
extern char	*ConfigPanel_GetTempdir(ptr_t idp);

extern int	ConfigPanel_SetDefaults (ptr_t idp);

#endif /* !_AUDIOTOOL_CONFIG_PANEL_H */
