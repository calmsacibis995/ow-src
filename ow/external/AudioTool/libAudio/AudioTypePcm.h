/* Copyright (c) 1990 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIOTYPEPCM_H
#define	_MULTIMEDIA_AUDIOTYPEPCM_H

#ident	"@(#)AudioTypePcm.h	1.4	92/07/21 SMI"

#include "AudioTypeConvert.h"


// This is the class for a linear PCM conversion module

class AudioTypePcm : public AudioTypeConvert {
protected:
	typedef unsigned char	ulaw;
	typedef unsigned char	alaw;

	// Conversion routines inline'd in the source file
	double char2dbl(char);
	double short2dbl(short);
	double long2dbl(long);
	long dbl2long(double, long);

	void char2short(char*&, short*&);
	void char2long(char*&, long*&);
	void char2float(char*&, float*&);
	void char2double(char*&, double*&);
	void char2ulaw(char*&, ulaw*&);
	void char2alaw(char*&, alaw*&);

	void short2char(short*&, char*&);
	void short2long(short*&, long*&);
	void short2float(short*&, float*&);
	void short2double(short*&, double*&);
	void short2ulaw(short*&, ulaw*&);
	void short2alaw(short*&, alaw*&);

	void long2char(long*&, char*&);
	void long2short(long*&, short*&);
	void long2float(long*&, float*&);
	void long2double(long*&, double*&);
	void long2ulaw(long*&, ulaw*&);
	void long2alaw(long*&, alaw*&);

	void float2char(float*&, char*&);
	void float2short(float*&, short*&);
	void float2long(float*&, long*&);
	void float2double(float*&, double*&);
	void float2ulaw(float*&, ulaw*&);
	void float2alaw(float*&, alaw*&);

	void double2char(double*&, char*&);
	void double2short(double*&, short*&);
	void double2long(double*&, long*&);
	void double2float(double*&, float*&);
	void double2ulaw(double*&, ulaw*&);
	void double2alaw(double*&, alaw*&);

	void ulaw2char(ulaw*&, char*&);
	void ulaw2short(ulaw*&, short*&);
	void ulaw2long(ulaw*&, long*&);
	void ulaw2float(ulaw*&, float*&);
	void ulaw2double(ulaw*&, double*&);
	void ulaw2alaw(ulaw*&, alaw*&);

	void alaw2ulaw(alaw*&, ulaw*&);
	void alaw2char(alaw*&, char*&);
	void alaw2short(alaw*&, short*&);
	void alaw2long(alaw*&, long*&);
	void alaw2float(alaw*&, float*&);
	void alaw2double(alaw*&, double*&);

public:
	AudioTypePcm();					// Constructor

	// Class AudioTypeConvert methods specialized here
	virtual Boolean CanConvert(			// TRUE if conversion ok
	    AudioHdr h) const;			// type to check against

	// Convert buffer to the specified type
	// Either the input or output type must be handled by this class
	virtual AudioError Convert(			// Convert to new type
	    AudioBuffer*& inbuf,		// data buffer to process
	    AudioHdr outhdr);			// target header

	virtual AudioError Flush(AudioBuffer*& buf);
};

#endif /* !_MULTIMEDIA_AUDIOTYPEPCM_H */
