/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)PlayRecord.cc	1.36	96/02/20 SMI"

#include <fcntl.h>
#include <math.h>
#include <unistd.h>
#include <stropts.h>

#include <multimedia/AudioLib.h>
#include <multimedia/tsm.h>
#include <multimedia/AudioTypePcm.h>
#include "PlayRecord.h"

#include "atool_i18n.h"
#include "atool_debug.h"

// #define USETSM			// Time Scale Modification for ff/rew 

// Play/Record manipulation routines

// Constructor
PlayRecord::
PlayRecord():
	dev(0), aptr(0), which(INACTIVE),
	inbuf(NULL), outbuf(NULL), tsmstate(NULL)	// XXX - temporary
{
}

// Destructor
PlayRecord::
~PlayRecord()
{
	if (which != INACTIVE)
		(void) stop();
	if (tsmstate) {
		tsm_destroy_state(tsmstate);
	}
}

// Initialize for asynchronous play of current list entry
// Caller should not call async() while this is executing.
AudioError PlayRecord::
play_init(
	char*		devname,	// device name
	Audio*		obj,		// audio object to play
	Double		lpos,		// left edge of range
	Double		rpos,		// right edge of range
	Double		startpos,	// starting position of play
	Double		speed)		// play velocity (<0 for reverse)
{
	AudioError	err;
	Double		curpos;
	unsigned	samp;
	AudioHdr	hdr;

	// Get current audio list
	obj->Reference();
	aptr = obj;

	// Speed must be valid
	if (speed == 0.)
		return (AUDIO_ERR_BADARG);

	// Cannot play backwards from undefined endpoint.
	if (speed < 0.) {
		if (Undefined(rpos))
			rpos = obj->GetLength();	// try to get endpt
		if (Undefined(rpos))
			return (AUDIO_ERR_BADARG);
	}

	// if no tsm state ptr, create one (yeah, it may not get used
	// at all, but just in case).
#ifdef USETSM
	if (!tsmstate) {
		tsmstate = tsm_create_state(aptr->GetHeader().sample_rate);
	}
#endif

	// If we're already playing, we need to determine whether or not the
	// new boundaries include the current play position.  If the play
	// position is not included, then we pause the play and then start
	// again from the new start position.  If the current position is
	// included, then we modify the finish point (dealing with what's
	// been queued, as appropriate).
	if (which != INACTIVE) {
		curpos = play_position();

		if ((speed == velocity) && (speed > 0.) &&
		    (startpos == AUDIO_UNKNOWN_TIME) &&
		    (curpos >= lpos) && (curpos <= rpos)) {
			// We are playing already, the speed is unchanged,
			// and the current position is within the new segment.
			// If the endpoint is not changing, we're done
			if (rightpos == rpos)
				return(AUDIO_SUCCESS);

			// Since the endpt is changing, clear existing EOF
			// to make sure it gets written again.
			rightpos = rpos;
			eofwritten = FALSE;
			eofseen = FALSE;

			if (rpos < (inpos - 0.001)) {
				// We already queued beyond the new segment end,
				// so flush the output and start from here.
				dev->Pause(WriteOnly);
				inpos = play_position();
				err = play_preroll();	// flushes first
				if (err)
					stop();
				return (err);
			} else {
				// Keep playing normally, extend endpoint.
				return (AUDIO_SUCCESS);
			}
		} else {
			// The segment and/or speed is changing.
			// Flush and start over.
			err = dev->Pause(WriteOnly);
			if (velocity != speed) {
				// reset the tsm state any time there's
				// a speed change
				if (tsmstate) {
					tsm_init_state(tsmstate);
				}
			}
		}
	} else {
		// Open output device.  Don't hang on open().
		dev = new AudioDevice(devname, WriteOnly);
		if (dev == NULL)
			return (AUDIO_UNIXERROR);
		dev->Reference();
		dev->SetBlocking(FALSE);
		err = dev->Open();
		if (err) {
			dev->Dereference();
			dev = NULL;
			aptr->Dereference();
			aptr = NULL;
			return (err);
		}
		dev->SetSignal(TRUE);
	}

	// Set state information
	// outpos is the device output 'position' which is relative to
	// the start of the data actually played.
	outpos = 0.;
	dev->SetWritePosition(outpos, Absolute);

	hdr = aptr->GetHeader(0.);
	if (!dev->CanSetHeader(hdr)) {
		err = AUDIO_ERR_HDRINVAL;
	} else {
		err = dev->SetHeader(hdr);
	}
	if (err) {
		dev->Dereference();
		dev = NULL;
		aptr->Dereference();
		aptr = NULL;
		return (err);
	}
 
 	// reset sample count
	samp = 0;
	(void) dev->SetPlaySamples(samp);

	leftpos = lpos;
	rightpos = rpos;
	velocity = speed;
	if (startpos != AUDIO_UNKNOWN_TIME) {
		if (velocity > 0.) {
			lpos = startpos;
		} else {
			rpos = startpos;
		}

		inpos = startpos;
		svstartpos = inpos;	// saved start position
	} else {
		svstartpos = inpos = curpos;
	}

	// If not normal speed, set up to scale and/or reverse
	// XXX - this should be implemented as a libaudio filter
	if (velocity != 1.) {
		// Allocate a half-second processing buffer
		// XXX - assumes uniform input data type
		if (inbuf == NULL) {
			inbuf = new AudioBuffer(.5);
			inbuf->SetHeader(aptr->GetHeader());
			inbuf->Reference();
		}
	}

	which = PLAY;
	eofwritten = FALSE;
	eofseen = FALSE;

	err = play_preroll();
	if (err)
		stop();
	return (err);
}

// Initialize for asynchronous record into a given Audio object
AudioError PlayRecord::
record_init(
	char*		devname,		// device name
	Audio*		obj,			// audio list
	Double		frompos,		// output start position
	Double		topos)			// end position
{
	AudioError	err;

	if (which != INACTIVE)
		return (AUDIO_ERR_NOEFFECT);

	// Get current audio list
	obj->Reference();
	aptr = obj;

	// Open input device.  Don't hang on open().
	dev = new AudioDevice(devname, ReadOnly);
	if (dev == NULL)
		return (AUDIO_UNIXERROR);
	dev->Reference();
	dev->SetBlocking(FALSE);
	err = dev->Open();
	if (!err) {
		err = dev->Pause(ReadOnly);
	}
	if (!err) {
		err = dev->Flush(ReadOnly);
	}
	if (!err) {
		err = dev->SetHeader(aptr->GetHeader(0.));
	}
	if (!err) {
		err = dev->Resume(ReadOnly);
	}
	if (err) {
		dev->Dereference();
		dev = NULL;
		aptr->Dereference();
		aptr = NULL;
		return (err);
	}
	dev->SetSignal(TRUE);

	// Set state information
	leftpos = frompos;
	rightpos = topos;
	svstartpos = frompos;	// saved start position
	inpos = 0.;
	outpos = frompos;
	velocity = 1.;
	which = RECORD;
	eofwritten = FALSE;
	eofseen = FALSE;

	// Kick start
	err = async();
	if (err == AUDIO_ERR_INTERRUPTED)
		err = AUDIO_SUCCESS;
	if (err)
		stop();
	return (err);
}


// Flush play/record queues
AudioError PlayRecord::
flush()
{
	AudioError	err;

	switch (which) {
	case PLAY:
		// Flush and reset output position
		err = dev->Flush(WriteOnly);
		outpos = dev->WritePosition();
		DBGOUT((5, "flush - PLAY: outpos=%7.2f\n", (double)outpos));
		break;
	case RECORD:
		// XXX - Should reset sample counter
		err = dev->Flush(ReadOnly);
		DBGOUT((5, "flush - RECORD\n"));
		break;
	default:
		return (AUDIO_ERR_NOEFFECT);
	}

	// Throw away processing buffer, if any
	if (outbuf != NULL) {
		outbuf->Dereference();
		outbuf = NULL;
	}
	return (err);
}

// Return TRUE if i/o has drained
Boolean PlayRecord::
eof()
{
	switch (which) {
	case PLAY:
	case RECORD:
		return (eofseen);
	}
	return (TRUE);
}

// Stop asynchronous play/record
AudioError PlayRecord::
stop()
{
	AudioError	err;

	switch (which) {
	case PLAY:
		endpos = play_position();
		DBGOUT((5, "stop - PLAY, endpos=%7.2f\n", (double)endpos));
		err = AUDIO_SUCCESS;
		break;
	case RECORD:
		// Pause recording and drain input to file
		err = dev->Pause(ReadOnly);
		if (!err) {
			// continue until quite finished
			do {
				err = async();
			} while (err == AUDIO_ERR_INTERRUPTED);
		}
		endpos = outpos;
		DBGOUT((5, "stop - RECORD, endpos=%7.2f\n", (double)endpos));
		break;
	default:
		return (AUDIO_ERR_NOEFFECT);
	}
	// Close and dereference audio file
	aptr->Dereference();
	aptr = NULL;

	// Close and dereference audio device
	dev->Close();
	dev->Dereference();
	dev = NULL;
	which = INACTIVE;

	// XXX - free any residual processing buffers
	if (inbuf != NULL) {
		inbuf->Dereference();
		inbuf = NULL;
	}
	if (outbuf != NULL) {
		outbuf->Dereference();
		outbuf = NULL;
	}
	return (err);
}

// Write out enough data to get a play going
AudioError PlayRecord::
play_preroll()
{
	AudioError	err;

	// Pause output and run a few async writes, ignoring continuation
	err = dev->Pause(WriteOnly);
	if (!err)
		err = flush();
	if (!err)
		err = async();
	if (err == AUDIO_ERR_INTERRUPTED)
		err = async();
	if (err == AUDIO_ERR_INTERRUPTED)
		err = async();
	if ((err == AUDIO_ERR_INTERRUPTED) || (err == AUDIO_EOF))
		err = AUDIO_SUCCESS;

	// XXX - Assume that the device will be closed if error
	if (err)
		flush();
	dev->Resume(WriteOnly);
	return (err);
}

// Asynchronous entry for play/record
// Returns AUDIO_ERR_INTERRUPTED if more data can be copied to/from the device
AudioError PlayRecord::
async()
{
	Double		startpos;
	Double		limit;
	Double		resid = 0.;
	AudioInfo	info;
	AudioError	err;
	unsigned	i;

	// Get device state
	err = dev->GetState(info);
	if (err)
		return (err);

	// Detect output eof if playing
	if (which == PLAY) {
		if (info->play.eof > 0) {
			DBGOUT((5, "async - PLAY: got EOF flag\n")); 

			// Reset eof flag - generates a SIGPOLL
			i = 0;
			(void) dev->SetPlayEof(i);

			if (eofwritten) {
#ifdef DEBUG_PRINT
				if (info->play.active) {
					fprintf(stderr,
					    "Play EOF with device active\n");
				} else
#endif
					// if eof and no more data
					eofseen = TRUE;
			}
		}
	} else if (which == RECORD) {
		if (info->record.error)
			return (AUDIO_ERR_DEVOVERFLOW);
	}

	// XXX - loop for now, since Copy paradigm is not worked out yet
	// XXX - except that we break out of the loop early to keep from
	//       locking down the UI too much

	startpos = inpos;
	do {
		// Calculate amount remaining to copy
		if (velocity > 0.) {
			if (Undefined(rightpos))
				limit = rightpos;
			else
				limit = rightpos - inpos;
		} else {
			// Playing backward
			limit = inpos - leftpos;
		}

		switch (which) {
		case PLAY:
			if ((outbuf == NULL) &&
			    (aptr->GetHeader().Time_to_Bytes(limit) == 0)) {
				err = AUDIO_EOF;
				limit = 0.;
				DBGOUT((5,
    "async - PLAY: reached end of play region with no AUDIO_EOF\n")); 

			} else if (velocity == 1.) {
				// Copy data to device
				DBGOUT((11,
    "async - PLAY: dev write startpos=%7.2f, outpos=%7.2f, limit=%7.2f\n",
			(double)startpos, (double)outpos, (double)limit)); 
				err = aptr->AsyncCopy(dev,
				    startpos, outpos, limit);

				// Update pointers
				if (!err &&
				    (err.sys == AUDIO_COPY_SHORT_OUTPUT)) {
					DBGOUT((5,
					    "async - PLAY: short write.\n"));
					limit = 0.;	// read > written
				}
				inpos = startpos;
				if (inpos >= rightpos)
					inpos = rightpos;

			} else if (outbuf == NULL) {
				// Non-standard velocity...
				// Process more input data
				// XXX - this processing should be in the API
				inbuf->SetLength(0.);
				if (velocity < 0.) {
					// If reverse, set ptrs accordingly
					startpos -=
					    min(limit, inbuf->GetSize());
				}

				// Read a segment of data and update ptrs
				DBGOUT((10,
    "async - PLAY: buffer read start=%5.2f, lim=%5.2f...",
    (double)startpos, (double)limit)); 
				err = aptr->Copy(inbuf, startpos, resid,
				    limit);
				if (!err &&
				    (err.sys == AUDIO_COPY_ZERO_LIMIT))
					err = AUDIO_EOF;
				if (err)
					break;
				if (velocity < 0.) {
					startpos -= limit;
					if (startpos <= leftpos)
						startpos = leftpos;
				} else {
					if (startpos >= rightpos)
						startpos = rightpos;
				}
				DBGOUT((10,
    "copied=%5.2f, newpos=%5.2f\n", (double)limit, (double)startpos)); 
				inpos = startpos;

				// If playing backward, reverse the data
				if (velocity < 0.) {
				}

				if (fabs(velocity) == 1.) {
					// no timescale...we're ready to output
					outbuf = inbuf;
					outbuf->Reference();
					outbuf_offset = 0.;
				} else {
#ifdef USETSM
					err = time_scale(inbuf, outbuf);
					// XXX check this later....
#else
					// XXX - tmp hack for speed > 1
					outbuf = inbuf;
					outbuf->SetLength(inbuf->GetLength() /
					    fabs(velocity));
					outbuf->Reference();
#endif
					outbuf_offset = 0.;
				}
			}

			// If there is any residual data in the
			// processing buffer, write it out now
			if (outbuf != NULL) {
				DBGOUT((11,
    "async - PLAY: buffer write start=%5.2f, outpos=%5.2f...",
    (double)outbuf_offset, (double)outpos)); 
				resid = AUDIO_UNKNOWN_TIME;
				err = outbuf->AsyncCopy(dev, outbuf_offset,
				    outpos, resid);
				DBGOUT((11,
    "copied=%5.2f, newstart=%5.2f\n",
    (double)resid, (double)outbuf_offset)); 
				if (!err &&
				    (err.sys == AUDIO_COPY_SHORT_OUTPUT)) {
					DBGOUT((11,
				    "async - PLAY: residual write - short.\n"));
					limit = 0.;	// more data in buffer
					break;
				}
				// Free output buffer
				outbuf->Dereference();
				outbuf = NULL;
#ifdef DEBUG_PRINT
				// XXX - should never happen??
				if ((err == AUDIO_EOF) && (limit != 0.)) {
					fprintf(stderr,
		     "async - PLAY: got AUDIO_EOF w/more data. resetting\n");
					err = AUDIO_SUCCESS;
				}
				// XXX - should never happen??
				if (err == AUDIO_EOF) {
					fprintf(stderr,
		     "async - PLAY: got AUDIO_EOF on buffer?!\n");
					err = AUDIO_SUCCESS;
				}
#endif
			}

			// Write an EOF record, if needed
			if ((err == AUDIO_EOF) && !eofwritten) {
					DBGOUT((5,
		     "async - PLAY: got AUDIO_EOF - writing EOF to device\n"));
				eofseen = FALSE;
				eofwritten = TRUE;	// Assume success
				err = dev->WriteEof();
				eofwritten = !err;
				// If no EOF written, try later
				if (err == AUDIO_ERR_NOEFFECT)
					err = AUDIO_EOF;
			}
			break;
		case RECORD:
			DBGOUT((11, "** got async: RECORD.\n"));
			if (limit == 0.) {
				DBGOUT((5,
			"async - RECORD: no more data, setting AUDIO_EOF\n"));
				err = AUDIO_EOF;
			} else {
				// Copy data from device
				DBGOUT((11,
	"async - RECORD: reading dev: spos=%7.2f, opos=%7.2f\n",
			(double)startpos, (double)outpos));

				err = dev->AsyncCopy(aptr,
				    startpos, outpos, limit);

				DBGOUT((11,
     "async - RECORD: after read: spos=%7.2f, opos=%7.2f, lim=%7.2f, err=%s\n",
		(double)startpos, (double)outpos, (double)limit, err.msg()));

				// Update pointers
				if (!err &&
				    (err.sys == AUDIO_COPY_SHORT_INPUT)) {
					limit = 0.;	// read > written
				}
				inpos = startpos;
				if (inpos >= rightpos) {
					inpos = rightpos;
				}
			}
			break;
		default:
			err = AUDIO_ERR_NOEFFECT;
			break;
		}
		// XXX - break out of loop to allow window notifier to run
		if (!err && (limit != 0.))
			return (AUDIO_ERR_INTERRUPTED);
	} while (!err && (limit != 0.));
	return (err);
}

// Return the current play position.  This corresponds to the
// actual output position, not queue time.
Double PlayRecord::
play_position()
{
	unsigned	samps;
	Double		pos;
	AudioInfo	info;
	AudioError	err;

	// If inactive, return the final position
	if (which == INACTIVE)
		return (endpos);

	// Get sample count
	samps = dev->GetPlaySamples();
	if (samps == AUDIO_UNKNOWN_SIZE) {
		return (AUDIO_UNKNOWN_TIME);
	}

	pos = svstartpos + (velocity *
	    dev->GetHeader().Samples_to_Time(samps));
	if (pos < 0.)
		pos = 0.;
	return pos;
}


// Return the current record position.  This corresponds to the
// actual input position, not queue time.
Double PlayRecord::
record_position()
{
	unsigned	samps;
	Double		pos;
	AudioInfo	info;
	AudioError	err;

	// If inactive, return the final position
	if (which == INACTIVE)
		return (endpos);

	// Get sample count
	samps = dev->GetRecSamples();
	if (samps == AUDIO_UNKNOWN_SIZE) {
		return (AUDIO_UNKNOWN_TIME);
	}

	pos = svstartpos + (velocity *
	    dev->GetHeader().Samples_to_Time(samps));
	if (pos < 0.)
		pos = 0.;
	return (pos);
}

// Return the current position of the next write in the record output
// buffer
Double PlayRecord::
record_bufposition()
{
	Double		pos;
	AudioError	err;

	return (outpos);
}

#ifdef USETSM
AudioError PlayRecord::
time_scale(AudioBuffer* inbuf, AudioBuffer*& output_buffer)
{
	AudioHdr	hdr;
	AudioTypePcm	conv;
	AudioHdr	newhdr;
	AudioBuffer	*tmpbuf = NULL;
	AudioBuffer	*outbuf = NULL;
	AudioError	err;
	unsigned int	nsamp, rsamp;

	// set hdr for converting. if it's PCM, no conversion necessary
	newhdr = hdr = inbuf->GetHeader();

	// XXX - it should always be created if we get this far
	if (!tsmstate) {
		tsmstate = tsm_create_state(hdr.sample_rate);
	}

	if ((newhdr.encoding != LINEAR) || (newhdr.bytes_per_unit != 2)) {
		newhdr = hdr;
		newhdr.encoding = LINEAR;
		newhdr.bytes_per_unit = 2;

		// create tmp buf, convert to PCM
		tmpbuf = new AudioBuffer();
		tmpbuf->SetHeader(inbuf->GetHeader());
		tmpbuf->SetSize(inbuf->GetLength());
		tmpbuf->SetLength(inbuf->GetLength());

		AudioCopy(inbuf, tmpbuf);

		err = conv.Convert(tmpbuf, newhdr);
		if (err) {
			return (err);
		}
		inbuf = tmpbuf;
	}
	
	outbuf = new AudioBuffer();
	// outbuf is same type as inbuf (PCM)
	outbuf->SetHeader(inbuf->GetHeader());
	// calculate & set size
	nsamp = tsm_outbuf_leng(inbuf->GetByteCount() / 
				inbuf->GetHeader().bytes_per_unit,
			        1. / fabs(velocity), tsmstate);
	outbuf->SetSize(outbuf->GetHeader().Samples_to_Time(nsamp));
	outbuf->SetLength(outbuf->GetSize());
	
	// XXX - is return val useful? looks like # bytes leftover. should
	// do outbuf->SetLength() to  correspond to this number
	rsamp = tsm((short *) (inbuf->GetAddress()),
	    inbuf->GetByteCount() / inbuf->GetHeader().bytes_per_unit,
	    1. / fabs(velocity), tsmstate,
	    (short *) (outbuf->GetAddress()));
	
	if (rsamp <= 0) {
#ifdef DEBUG_PRINT
		fprintf(stderr, "warning: tsm() returns 0!\n");
#endif DEBUG
	} else if (rsamp != nsamp) {
		DBGOUT((3, MGET("warning: tsm() returns %d (expect %d)!\n"),
		    rsamp, nsamp));
		outbuf->SetLength(outbuf->GetHeader().Samples_to_Time(rsamp));
	}

	// new cvt back to ULAW, if that's what we started with
	if ((hdr.encoding != newhdr.encoding) && 
	    (hdr.bytes_per_unit != newhdr.bytes_per_unit)) {
		err = conv.Convert(outbuf, hdr);
		if (err) {
			return (err);
		}
	}

	if (tmpbuf) {
		delete(tmpbuf);
	}

	outbuf->Reference();
	output_buffer = outbuf;	// pass it back up ...

	return (AUDIO_SUCCESS);
}
#endif /* USETSM */
