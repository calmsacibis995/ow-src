/* Copyright (c) 1991 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIO_AGC_H
#define	_MULTIMEDIA_AUDIO_AGC_H

#ident	"@(#)audio_agc.h	1.3	92/03/20 SMI"


/* Define the 'peak' value as the peak ulaw value mapped to floating-point */
#include <multimedia/libaudio.h>
#include <multimedia/audio_encode.h>

#define	_PEAK_VALUE_	(audio_s2d(audio_u2s(~0x7f)))


/* Define a state structure for automatic gain control */
typedef struct {
	unsigned	size;		/* number of samples per buffer */
	unsigned	clipcnt;	/* number of samples that clipped */
	unsigned	waslo;		/* count of low input buffers */
	double		elapsed;	/* elapsed time accumulator */
	double		sum;		/* sum of squares accumulator */
	double		peaksum;	/* peak sum of squares */
	double		spp;		/* seconds per period */
	double		sumavg;		/* sum of window values */
	unsigned	avgcnt;		/* number of points in sum */
	int		hysteresis;	/* ctr for gain increases */
	double*		inbuf;		/* input data buffer */
	double*		outbuf;		/* output sum of squares buffer */
	double*		svsamps;	/* saved squares buffer */
} Audio_agc;

/* Declare automatic gain control routines */
Audio_agc*	audio_agcinit(unsigned, unsigned);
void		audio_agcfree(Audio_agc*);
int		audio_agc(Audio_agc*, unsigned char*);

#endif /* !_MULTIMEDIA_AUDIO_AGC_H */
