/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_RESAMPLE_H
#define	_MULTIMEDIA_RESAMPLE_H

#ident	"@(#)Resample.h	1.2	92/11/10 SMI"

/*
 * To convert the sampling rate of fi of input signal to fo of output signal,
 * the least common multiple fm = L * fi = M * fo needs to be derived first.
 * Then the input signal is
 * 1) up-sampled to fm by inserting (L - 1) zero-valued samples after each
 *    input sample,
 * 2) low-pass filtered to half of the lower of fi and fo, and
 * 3) down-sampled to fo by saving one out of every M samples.
 *
 * The low-pass filter is implemented with an FIR filter which is a truncated
 * ideal low pass filter whose order is dependent on its bandwidth.
 *
 * Refer to Fir.h for explanations of filter() and flush().
 *
 */
#include "Fir.h"

class ResampleFilter : public Fir {
	int	num_state;		// interpolation requires less states
	int	up;			// upsampling ratio
	int	down;			// down sampling ratio
	int	down_offset;		// 0 <= index in down-sampling < down
	int	up_offset;		// -up < index in up_sampling <= 0

	void updateState(double *in, int size);

	// if fi is a multiple of fo, LP filtering followed by down_sampling
	int decimate_noadjust(short *in, int size, short *out);
	int decimate_flush(short *);
	int decimate(short *in, int size, short *out);

	// if fo is a multiple of fi, up-sampling followed by LP filtering
	int interpolate_noadjust(short *in, int size, short *out);
	int interpolate_flush(short *);
	int interpolate(short *in, int size, short *out);

	int flush(short *out);

public:
        ResampleFilter(int rate_in, int rate_out);
	virtual int	filter_noadjust(short *in, int size, short *out);
	virtual int	filter(short *in, int size, short *out);
	virtual int	getFlushSize(void);
};

#endif /* !_MULTIMEDIA_RESAMPLE_H */
