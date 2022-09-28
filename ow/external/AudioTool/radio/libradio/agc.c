/* Copyright (c) 1991 by Sun Microsystems, Inc. */
#ident	"@(#)agc.c	1.3	92/10/21 SMI"

#include <math.h>
#include "audio_agc.h"

/* agc parameters */
static double	Hisignal = .032;
static double	Lowsignal = .01;
static double	Nosignal = .001;

/* This defines the minimum number of seconds between gain increases */
static unsigned	Hysteresis = 5;


/* Free an agc state structure */
void
audio_agcfree(
	Audio_agc*	ap)
{
	if (ap != NULL) {
		if (ap->inbuf != NULL)
			(void) free((char *)ap->inbuf);
		if (ap->outbuf != NULL)
			(void) free((char *)ap->outbuf);
		if (ap->svsamps != NULL)
			(void) free((char *)ap->svsamps);
		(void) free((char *)ap);
	}
}

/* Allocate and initialize an agc state structure */
Audio_agc*
audio_agcinit(
	unsigned	size,		/* number of samples per audio_agc() */
	unsigned	rate)		/* sample rate */
{
	Audio_agc*	ap;

	/* Allocate structure and buffer */
	ap = (Audio_agc*)calloc(1, sizeof (*ap));
	if (ap == NULL)
		return (NULL);
	ap->inbuf = (double*) calloc(size, sizeof (double));
	ap->outbuf = (double*) calloc(size, sizeof (double));
	ap->svsamps = (double*) calloc(size, sizeof (double));
	if (ap->svsamps == NULL) {
		audio_agcfree(ap);
		return (NULL);
	}

	/* Init other state */
	ap->size = size;
	ap->spp = (double)ap->size / (double)rate;
	return (ap);
}


/*
 * Process the input data buffer (floating-point data values),
 * producing a sum of squares output buffer using a moving window of
 * 'ap->size' samples.  Sums are normalized to the buffer size.
 * The first 'ap->size - 1' sums are naturally going to be of little interest.
 *
 * Returns the peak value in the input buffer (1.0 if there were two
 * consecutive values greater than _PEAK_VALUE_ (the max ulaw value)).
 */
static double
audio_agcwindow(
	Audio_agc*	ap,
	double*		in,
	double*		out)
{
	int		i;
	int		lastpeak;
	double		val;
	double		peak;

	lastpeak = FALSE;
	ap->clipcnt = 0;
	peak = 0.;

	/* loop through the input buffer */
	for (i = 0; i < ap->size; i++) {
		val = fabs(*in++);

		if (val > peak) {
			peak = val;	/* save peak value */
		}
		if (val >= _PEAK_VALUE_) {
			if (lastpeak) {
				peak = 1.0;
				ap->clipcnt++;
			} else {
				lastpeak = TRUE;
			}
		} else {
			lastpeak = FALSE;
		}

		val *= val;
		ap->sum += val;
		ap->sum -= ap->svsamps[i];
		ap->svsamps[i] = val;	/* save value to subtract later */
		*out = (ap->sum / (double)ap->size);
		if (*out > ap->peaksum)
			ap->peaksum = *out;	/* save peak sum of squares */
		out++;
	}
	return (peak);
}


/*
 * Process an input u-law data buffer (containing ap->size samples).
 * Returns:
 *	10: no signal detected
 *	 1: gain should be raised a little
 *	 0: gain is ok
 *	-1: gain should be lowered a little
 *	-2: gain should be lowered some more
 *	-10:gain should be lowered dramatically (negative values scale)
 */
int
audio_agc(
	Audio_agc*	ap,
	unsigned char*	in)
{
	int		i;
	int		retval;
	double		peak;
	double		val;
	double		avg;

	/* Convert data to floating point */
	for (i = 0; i < ap->size; i++) {
		ap->inbuf[i] = audio_s2d(audio_u2s(in[i]));
	}

	/* Compute sums of squares and average them */
	peak = audio_agcwindow(ap, ap->inbuf, ap->outbuf);

	/* If clipping, reset params and lower the volume */
	if (ap->clipcnt > 2) {
		ap->waslo = 0;
		ap->elapsed = 0.;
		ap->sumavg = 0.;
		ap->avgcnt = 0;
		/*
		 * Compute a gain scaling that maps a clip count of 2 to -2
		 * and a clip count of size/16 to -10.
		 */
		i = (ap->size / 16) + 1;
		i = (((int)ap->clipcnt * -8) / i) - 2;
		if (i < -20)
			i = -20;
		return (i);
	}

	/* Average all the sums of squares */
	for (i = 0; i < ap->size; i++) {
		/* Only average in reasonable levels */
		if (ap->outbuf[i] >= Nosignal) {
			ap->sumavg += ap->outbuf[i];
			ap->avgcnt++;
		}
	}

	/* Add increment to time counter */
	ap->elapsed += ap->spp;
	retval = 0;

	/* If 1 second has elapsed (at least) */
	if (ap->elapsed >= 1.) {
		/*
		 * Calculate overall average sum of squares.
		 * If actual number of averages is close to what's expected,
		 * the signal was fairly constant (eg, music).
		 * If not, weight the signal a bit (eg, speech).
		 */
		avg = ap->sumavg / (double)ap->avgcnt;
		val = (ap->elapsed / ap->spp) * (double)ap->size;
		if ((val - (double)ap->avgcnt) > 10)
			avg += Lowsignal / 2.;

		if (ap->peaksum < Nosignal) {
			/* No detected input */
			retval = 10;

		} else if (ap->peaksum < Lowsignal) {
			/* Signal was low, should be raised a bit at a time */
			if ((++ap->waslo > 1) && (ap->hysteresis-- < 0)) {
				retval = 1;
				ap->hysteresis = Hysteresis;
			}
		} else {
			/* Signal was detectable */
			ap->waslo = 0;
			if (avg >= Hisignal) {
				/* Drop the gain a little */
				retval = -1;
			}
		}

		/* Update accumulators */
		ap->peaksum = 0.;
		ap->elapsed = 0.;
		ap->sumavg = 0.;
		ap->avgcnt = 0;
		if (ap->hysteresis > 0)
			ap->hysteresis--;
	} else {
		/* Inbetween buffers, detect no signal only */
		if (ap->peaksum < Nosignal) {
			/* No detected input */
			retval = 10;
		}
	}

	return (retval);
}
