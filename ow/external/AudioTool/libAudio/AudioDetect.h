/* Copyright (c) 1990 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIODETECT_H
#define	_MULTIMEDIA_AUDIODETECT_H

#ident	"@(#)AudioDetect.h	1.7	97/01/14 SMI"

/* Audio sound/silence detection structures and routines */


/*
 * An array of AudioDetectPts is returned by the detection algorithms.
 * 'Pos' gives the start time of a region.  'Type' give the type of data
 * detected in that particular region.  The end time of the region is equal
 * to the start time of the next region.
 * The last entry in the array has type DETECT_EOF.
 */
enum AudioDetectType {
    DETECT_EOF,			/* end of file marker */
    DETECT_SILENCE,		/* region of silence */
    DETECT_SOUND		/* region of detected sound */
			/* XXX - may be extended to include */
			/* touch-tone and speech detection. */
};
struct AudioDetectPts {
	double			pos;	/* start time of audio region */
	enum AudioDetectType	type;	/* type of audio region */
};

/* Audio detection algorithm tuning parameters */
enum AudioDetectConfig {
	DETECT_MINIMUM_SILENCE,		/* (double) minimum pause time */
	DETECT_MINIMUM_SOUND,		/* (double) minimum sound time */
	DETECT_THRESHOLD_SCALE,		/* (double) 0.(low) - 4.(hi) */
	DETECT_NOISE_RATIO		/* (double) 0.(low) - 1.(hi) */
};


/* The rest of this is only defined if compiling C++ */
#ifdef __cplusplus

#include "Audio.h"

// Array of detection points
class AudioDetectArray {
private:
	unsigned int	count;		// number of valid points in array
	unsigned int	size;		// number of entries in array

	void operator=(AudioDetectArray);		// Assignment is illegal
public:
	AudioDetectPts*	pts;		// array ptr

	AudioDetectArray(unsigned int cnt=0);		// Constructor
	~AudioDetectArray();				// Destructor

	AudioError appendpts(				// Append points
	    AudioDetectPts* newpts,		// array to append
	    unsigned int cnt=-1);		// number of points to append
	void reduce();					// Eliminate redundancy
	AudioError duparray(AudioDetectPts*& cp);	// Copy array
};


// Audio detection state structure
class AudioDetect {
private:
	void*		state;		// detection algorithm state
	Double		min_silence;	// length of minimum silence segment
	Double		min_sound;	// length of minimum sound segment
	Double		thresh_scale;	// silence threshold scale
	Double		noise_ratio;	// silence threshold noise ratio

	void operator=(AudioDetect);			// Assignment is illegal

	AudioError analyzeappend(			// Analyze main loop
	    AudioDetectArray*& aptr,		// value array to modify
	    Audio* obj,				// AudioList, or whatever
	    Double from,			// starting offset
	    Double to,				// ending offset
	    Double mintime);			// minimum analysis buffer size
public:
	AudioDetect();					// Constructor
	~AudioDetect();					// Destructor

	AudioError GetParam(				// Get detection params
	    AudioDetectConfig type,		// type flag
	    Double& val);			// return value

	AudioError SetParam(				// Set detection params
	    AudioDetectConfig type,		// type flag
	    Double val);			// value

	AudioError SetNoiseFloor(			// Analyze silence
	    Audio* obj);			// AudioExtent, or whatever

	AudioError AnalyzeGain(				// Analyze gain level
	    Audio* obj,				// AudioExtent, or whatever
	    Double& val);			// return value

	AudioError Analyze(				// Process data
	    AudioDetectPts*& pts,		// value array to modify
	    Audio* obj);			// AudioList, or whatever

	AudioError Analyze(				// Process data
	    AudioDetectPts*& pts,		// value array to modify
	    Audio* obj,				// AudioList, or whatever
	    Double from);			// starting offset

	AudioError Analyze(				// Process data
	    AudioDetectPts*& pts,		// value array to modify
	    Audio* obj,				// AudioList, or whatever
	    Double from,			// starting offset
	    Double to);				// ending offset
};

#endif /*__cplusplus*/

#endif /* !_MULTIMEDIA_AUDIODETECT_H */
