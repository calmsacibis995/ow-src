/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIOGAIN_H
#define	_MULTIMEDIA_AUDIOGAIN_H

#ident	"@(#)AudioGain.h	1.8	93/01/07 SMI"

#include "AudioTypePcm.h"

// Class to handle gain calculations


// Define bits for AudioGain::Process() type argument
#define	AUDIO_GAIN_INSTANT	(1)		// Gain for level meter
#define	AUDIO_GAIN_WEIGHTED	(2)		// Gain for agc


class AudioGain {
protected:

static const double	LoSigInstantRange;	// normalization constants
static const double	HiSigInstantRange;
static const double	NoSigWeight;
static const double	LoSigWeightRange;
static const double	HiSigWeightRange;
static const double	PeakSig;
static const double	DCtimeconstant;		// DC offset time constant

	AudioTypePcm	float_convert;		// used in signal processing
	unsigned	clipcnt;		// clip counter
	Double		DCaverage;		// weighted DC offset
	Double		instant_gain;		// current (instantaneous) gain
	Double		weighted_peaksum;	// peak weighted sum
	Double		weighted_sum;		// running sum of squares
	Double		weighted_avgsum;	// accumulated sums to averages
	unsigned	weighted_cnt;		// number of sums to average
	double*		gain_cache;		// weighted gains
	Double		gain_cache_size;	// number of cached gains

protected:
	// Internal processing methods
	virtual void process_dcfilter(		// filter DC bias
	    AudioBuffer*);
	virtual void process_instant(		// calculate instant gain
	    AudioBuffer*);
	virtual void process_weighted(		// calculate weighted gain
	    AudioBuffer*);

public:
		AudioGain();			// Constructor
	virtual	~AudioGain();			// Destructor

	virtual Boolean CanConvert(		// TRUE if conversion ok
	    const AudioHdr&) const;		// type to check against

	virtual AudioError Process(		// Process new audio data
	    AudioBuffer*, int);		// buffer destroyed if not referenced!
	virtual double InstantGain();		// Get most recent gain
	virtual double WeightedGain();		// Get current weighted gain
	virtual double WeightedPeak();		// Get peak weighted gain
	virtual Boolean Clipped();		// TRUE if peak since last check
	virtual void Flush();			// Reset state
};

#endif /* !_MULTIMEDIA_AUDIOGAIN_H */
