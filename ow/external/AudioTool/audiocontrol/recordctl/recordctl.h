/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _AUDIOCONTROL_RECORDCTL_H
#define	_AUDIOCONTROL_RECORDCTL_H

#ident	"@(#)recordctl.h	1.8	93/06/08 SMI"

#include <multimedia/AudioDevice.h>
#include <multimedia/AudioGain.h>
#include "audioctl.h"


const int	MAX_INPUTS = 3;		// Maximum number of input ports

// Flags for level meter hi/lo signal indicators
#define	LEVEL_NO_SIGNAL		(1)
#define	LEVEL_LO_SIGNAL		(2)
#define	LEVEL_PEAK_SIGNAL	(4)


// Record Control Panel base class (toolkit-independent)

class Recordctl : public Audioctl {
private:
	// Input port name strings
	char*		MICROPHONE_STRING;
	char*		LINEIN_STRING;
	char*		INTERNALCD_STRING;
	char*		INTERNAL_STRING;

static const double	NoSig;			// Signal ranges
static const double	LoSig;
static const double	HiSig;
static const double	AvgLoSig;
static const double	AvgHiSig;
static const double	AdjustTime;		// time between adjustments
static const double	SettleTime;		// duration of normal gain

	AudioDevice*	audiodev;		// Device for auto-adjust
	AudioGain	gainconvert;		// Object for gain conversions
	AudioHdr	savehdr;		// Format at start of adjust
	Boolean		sethigh;		// TRUE if volume set to max
	Double		agc_buflen;		// Length of input buffering
	Double		agc_time;		// Auto-adjust latency
	Double		settle_time;		// Auto-adjust settle time

public:
	Boolean		adjusting;		// TRUE: auto-adjust in-progress
	unsigned int	update_frequency;	// number of updates per sec

protected:
	virtual void	UpdateVolume();			// update agc/level

	// Methods that must be specialized
	virtual void	update_display(AudioInfo&) = 0;	// update panel
	virtual void	setbalance(Boolean) = 0;	// enable/disable
	virtual void	setinports(DevicePort*) = 0;	// set ports

	virtual void	SetAdjustmsg(char* = NULL) = 0;	// set msg string
	virtual void	SetErrormsg(char* = NULL) = 0;	// display error msg
	virtual void	SetLevelmeter(double, int = 0) = 0;	// set led meter
	virtual void	StartAdjust();			// start auto-adjust
	virtual void	StopAdjust(int=0);		// stop auto-adjust

public:
			Recordctl(ptr_t=NULL);		// Constructor
	virtual		~Recordctl();			// Destructor

	// Recordctl methods for device control
	virtual void	SetVolume(double);		// set volume
	virtual void	VolumeIncr(Boolean);		// incr/decr volume
	virtual void	SetMonitor(double);		// monitor volume
	virtual void	MonitorIncr(Boolean);		// incr/decr monitor
	virtual void	SetBalance(double);		// set balance
	virtual void	SetPort(unsigned, Boolean);	// change inport

	// Public methods
	virtual int	Setdevice(char*);		// set device name
	virtual void	Update();			// update display
};

// Global routine to instantiate a toolkit-dependent panel
extern Recordctl*	Create_Recordctl(char*, ptr_t);


#endif /* !_AUDIOCONTROL_RECORDCTL_H */
