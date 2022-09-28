/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _AUDIOTOOL_PLAYRECORD_H
#define	_AUDIOTOOL_PLAYRECORD_H

#ident	"@(#)PlayRecord.h	1.11	92/12/14 SMI"

#include <multimedia/AudioBuffer.h>
#include <multimedia/AudioDevice.h>
#include <multimedia/tsm.h>


// Define a play/record class
class PlayRecord {
private:
	AudioDevice*	dev;		// audio device
	Audio*		aptr;		// audio file
	Double		inpos;		// current input position
	Double		outpos;		// current output position
	Double		svstartpos;	// saved start of input position
	Double		endpos;		// final file position
	Double		leftpos;	// start of audio segment
	Double		rightpos;	// end of audio segment
	Double		velocity;	// play velocity
	Boolean		eofwritten;	// TRUE: eof flag written out
	Boolean		eofseen;	// TRUE: eof flag seen
	AudioError	errseen;	// last error
	AudioBuffer*	inbuf;		// XXX - input filter buffer
	AudioBuffer*	outbuf;		// XXX - output filter buffer
	Double		outbuf_offset;	// XXX - partial write position
	enum { INACTIVE, PLAY, RECORD, SCRUB }
			which;		// current operation type

	TSM_STATE	*tsmstate;	// for time scaling

	void operator=(PlayRecord);		// Assignment is illegal
public:
	PlayRecord();				// Constructor
	~PlayRecord();				// Destructor
	AudioError play_init(			// Init for playing
	    char* devname,		// device name
	    Audio* obj,			// audio list
	    Double lpos,		// left edge of range
	    Double rpos,		// right edge of range
	    Double startpos,		// start position
	    Double speed);		// velocity (<0 for reverse play)
	AudioError record_init(			// Init for recording
	    char* devname,		// device name
	    Audio* obj,			// audio list
	    Double frompos,		// output start position
	    Double topos);		// end position
	AudioError flush();			// Flush play/record
	AudioError stop();			// Stop play/record
	AudioError async();			// Async entry
	AudioError play_preroll();		// Queue up initial output
	Boolean eof();				// TRUE if i/o drained
	Double play_position();			// Get play position
	Double record_position();		// Get record position
	Double record_bufposition();		// Get record buffer position
	AudioError time_scale(AudioBuffer*, AudioBuffer*&); // time scale
};

#endif /* !_AUDIOTOOL_PLAYRECORD_H */
