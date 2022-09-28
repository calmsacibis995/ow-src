/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _AUDIOCONTROL_H
#define	_AUDIOCONTROL_H

#ident	"@(#)audiocontrol.h	1.12	93/02/04 SMI"

#include <stropts.h>
#include "audio_i18n.h"
#include "audio_types.h"

#define	STRCPY		(void) strcpy
#define	SPRINTF		(void) sprintf

/* localization stuff */
extern char*		I18N_Message_File;

#ifndef SUNOS41
#define	MGET(str)	(char*) gettext(str)

#else /* 4.x */
#define MGET(str)	str
#define	textdomain(d)
#endif /* 4.x */


/* Procedures needed by panel and/or tooltalk code */
extern void	Audioctl_map_recpanel_only();
extern void	Audioctl_open_playpanel();
extern void	Audioctl_show_playpanel();
extern void	Audioctl_show_recpanel();
extern void	Audioctl_show_statuspanel();
extern void	Audioctl_recpanel_exit();
extern void	Audioctl_set_device(char*);
extern int	Audioctl_set_quit_timer(int);
extern void*	Audioctl_get_playpanel();
extern void*	Audioctl_get_recpanel();
extern void	Audioctl_dstt_quit();
extern int	Audioctl_tt_started();

#endif /* !_AUDIOCONTROL_H */
