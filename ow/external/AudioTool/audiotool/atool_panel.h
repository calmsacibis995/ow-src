/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _AUDIOTOOL_ATOOL_PANEL_H
#define	_AUDIOTOOL_ATOOL_PANEL_H

#ident	"@(#)atool_panel.h	1.22	92/12/14 SMI"

#include <multimedia/libaudio_c.h>
#include "atool_types.h"

extern char *Progname;		/* global program name for error messages */

/*
 * XXX
 * this floating point comparision stuff should be supported by
 * the API.  Right now, I just assume ulaw.  To support CD quality,
 * the model for supporting different sampling rates needs to be 
 * clarified and implemented.  
 */
#define XXX_TIME_GRANULARITY_ULAW	.000125
#define XXX_TIME_GRANULARITY_PCM	.0000227

/* THESE ARE INTENDED FOR ULAW ONLY */
#define SECS_EQUAL(s1, s2)	(fabs(s1 - s2) < XXX_TIME_GRANULARITY_ULAW)
#define SECS_GT(s1, s2)		(!SECS_EQUAL(s1,s2) && (s1 > s2))
#define SECS_LT(s1, s2)		(!SECS_EQUAL(s1,s2) && (s1 < s2))

/* Define public interfaces */
extern ptr_t		AudPanel_Init(ptr_t, int*, char**);
extern ptr_t		AudPanel_Gethandle(ptr_t);
extern void		AudPanel_Show(ptr_t);
extern void		AudPanel_Unshow(ptr_t);
extern double		AudPanel_Seek(ptr_t, double);
extern void		AudPanel_Playspeed(ptr_t, double);
extern void		AudPanel_Getpath(ptr_t, char*, int);
extern int		AudPanel_Loadfile(ptr_t, char*, int);
extern int		AudPanel_Unloadfile(ptr_t, int, Audio_hdr*);
extern ptr_t		AudPanel_Getowner(ptr_t);
extern void		AudPanel_Selectall(ptr_t);
extern int		AudPanel_Setpointer(ptr_t, double);
extern void		AudPanel_Reset(ptr_t);
extern void		AudPanel_Quit(ptr_t);
extern void		AudPanel_Hide(ptr_t);
extern void		AudPanel_Busy(ptr_t);
extern void		AudPanel_Unbusy(ptr_t);
extern void		AudPanel_AsyncBusy(ptr_t);
extern void		AudPanel_AsyncUnbusy(ptr_t);
extern void		AudPanel_Errorbell(ptr_t);
extern void		AudPanel_Errormessage(ptr_t, char*);
extern void		AudPanel_Message(ptr_t, char*);
extern void		AudPanel_Alert(ptr_t, char*);
extern int		AudPanel_Choicenotice(ptr_t, char*,
			    char*, char*, char*);
extern void		AudPanel_Displaysize(ptr_t);
extern int		AudPanel_Isicon(ptr_t);
extern int		AudPanel_Ismodified(ptr_t);
extern int		AudPanel_Iscompose(ptr_t);
extern Audio_Object	AudPanel_Getshelf(ptr_t);
extern void		AudPanel_Namestripe(ptr_t);
extern void		AudPanel_Setlink(ptr_t, char*, int);
extern char*		AudPanel_Getlink(ptr_t);
extern void		AudPanel_Breaklink(ptr_t);

#endif /* !_AUDIOTOOL_ATOOL_PANEL_H */
