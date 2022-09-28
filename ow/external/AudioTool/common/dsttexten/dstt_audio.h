/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_DSTT_AUDIO_H
#define	_MULTIMEDIA_DSTT_AUDIO_H

#ident	"@(#)dstt_audio.h	1.2	92/11/03 SMI"

#include "dstt.h"

/* add our own stuff that would normally go in dstt.h */

/* message name */
#define AUDIO_CONTROL		"Audio_Control"

#ifdef __cplusplus
extern "C" {
#endif

typedef int	Audioctl_CB(Tt_message, void *, int,
			    char *, void *, int,
			    char *,
			    char *, char *);

extern	int	dstt_audioctl(Audioctl_CB *, void *,
			      char *,	void *, int,
			      char *,
			      char *, char *);

extern	int dstt_audioctl_callback(char *, ...);
extern	int dstt_audioctl_register(char *, ...);

#ifdef __cplusplus
};
#endif

#endif /* !_MULTIMEDIA_DSTT_AUDIO_H */
