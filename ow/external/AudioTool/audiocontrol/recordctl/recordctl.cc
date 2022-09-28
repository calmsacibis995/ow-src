/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)recordctl.cc	1.14	96/02/20 SMI"

#include <stdio.h>
#include <unistd.h>

#include <multimedia/AudioDevice.h>
#include <multimedia/AudioBuffer.h>
#include "recordctl.h"


// Record Control Panel implementation

// Initialize signal range limits
const double	Recordctl::NoSig = .0001;
const double	Recordctl::LoSig = .05;
const double	Recordctl::HiSig = .95;
const double	Recordctl::AvgLoSig = .12;
const double	Recordctl::AvgHiSig = .45;
const double	Recordctl::AdjustTime = .75;
const double	Recordctl::SettleTime = 3.5;


// Constructor
Recordctl::
Recordctl(
	ptr_t		p):
	Audioctl(p), adjusting(FALSE), update_frequency(20)
{
	// Init input port names
	MICROPHONE_STRING = MGET("Microphone");
	LINEIN_STRING = MGET("Line In");
	INTERNALCD_STRING = MGET("Internal CD");
	INTERNAL_STRING = MGET("Internal");
}

// Destructor
Recordctl::
~Recordctl()
{
	if (audiodev != NULL)
		delete audiodev;
}

// Start an Auto-Adjust
void Recordctl::
StartAdjust()
{
	AudioError	err;
	AudioHdr	hdr;

	if (adjusting)
		return;		// already started
	if (ctldev == NULL) {
		SetErrormsg(MGET("No audio device"));
		return;
	}

	// Save the audio format to restore on Stop (preserves monitor format)
	savehdr = ctldev->GetReadHeader();
	hdr = savehdr;

	// Open the audio device readonly
	audiodev = new AudioDevice(Getdevice(), ReadOnly);
	audiodev->SetBlocking(FALSE);
	err = audiodev->Open();
	if (err != AUDIO_SUCCESS) {
		if (err == AUDIO_ERR_DEVICEBUSY) {
			SetErrormsg(MGET("The audio input device is busy"));
		} else {
			SetErrormsg(MGET("No audio device"));
		}
		delete audiodev;
		audiodev = NULL;
		return;
	}

#ifdef BUG_1113580
	// XXX - set the format to one that is likely to work
#ifdef SUNOS41
	hdr.sample_rate = 8000;
#endif
	hdr.encoding = ULAW;
	hdr.samples_per_unit = 1;
	hdr.bytes_per_unit = 1;
	hdr.channels = 1;
#endif /* BUG_1113580 workaround */

	// Make an attempt to preserve the audio format
	if (gainconvert.CanConvert(hdr)) {
		audiodev->Pause(ReadOnly);
		audiodev->Flush(ReadOnly);
		(void) audiodev->SetReadHeader(hdr);
		audiodev->Resume(ReadOnly);
	}

	// Get current data format and make sure we can process it
	hdr = audiodev->GetReadHeader();
	if (!gainconvert.CanConvert(hdr)) {
		SetErrormsg(MGET("The audio device is busy"));
		delete audiodev;
		audiodev = NULL;
		return;
	}

	// Try to set the input buffering to the update rate
	sethigh = FALSE;
	settle_time = 0.;
	agc_time = 0.;
	agc_buflen = (1. / (double) update_frequency);
	(void) audiodev->SetRecDelay(agc_buflen);
	(void) audiodev->SetSignal(TRUE);

	// We're ready to go!  The rest is handled in the GUI-dependent code
	// XXX - test for line in vs mic?  test for missing mic?
	SetErrormsg();
	SetAdjustmsg();
	adjusting = TRUE;
}

// Stop an Auto-Adjust
void Recordctl::
StopAdjust(
	int)
{
	if (!adjusting)
		return;		// already stopped

	// Make an attempt to reset the audio format
	(void) audiodev->SetReadHeader(savehdr);

	// Close the device.  The rest is handled in the GUI-dependent code
	delete audiodev;
	audiodev = NULL;
	gainconvert.Flush();
	adjusting = FALSE;
}

// Set the device name for the tool
int Recordctl::
Setdevice(
	char*	devname)
{
	int	err;

	err = Audioctl::Setdevice(devname);
	if ((err >= 0) && adjusting) {
		// Cancel auto-adjust, if in-progress when device name changes
		StopAdjust();
	}
	return (err);
}


// Get the latest device state and update the panels appropriately.
void Recordctl::
Update()
{
	AudioInfo	newinfo;
	AudioInfo	change;
	Boolean		flag;
	int		i;
	DevicePort	inputs[MAX_INPUTS];
	unsigned	input_avail;

	if (ctldev == NULL) {
		info.Clear();
		return;
	}

	// Get the current device state
	(void) ctldev->GetState(newinfo);

	// XXX - patch available ports for 4.x AMD devices
	if ((type == AudioDeviceAMD) &&
	    ((newinfo->record.avail_ports == 0) ||
	    (newinfo->record.avail_ports == (unsigned)~0))) {
		newinfo->record.avail_ports = AUDIO_MICROPHONE;
	}

	// If the device type may have changed, get the new type
	// and update the panel appearance to match
	if (type == (AudioDeviceType) -1) {
		type = ctldev->GetDeviceType();

		// XXX - backwards compatability for 4.x AMD devices
		if ((type == AudioDeviceAMD) &&
		    ((newinfo->record.avail_ports == 0) ||
		    (newinfo->record.avail_ports == (unsigned)~0))) {
			newinfo->record.avail_ports = AUDIO_MICROPHONE;
		}

		// Initialize the input port names
		i = 0;
		input_avail = newinfo->record.avail_ports;
		if (input_avail & AUDIO_MICROPHONE) {
			inputs[i].name = MICROPHONE_STRING;
			inputs[i++].port = AUDIO_MICROPHONE;
			input_avail &= ~AUDIO_MICROPHONE;
		}
		if (input_avail & AUDIO_LINE_IN) {
			inputs[i].name = LINEIN_STRING;
			inputs[i++].port = AUDIO_LINE_IN;
			input_avail &= ~AUDIO_LINE_IN;
		}

#ifdef AUDIO_INTERNAL_CD_IN
		// Support for built-in CD analog audio input
		if (input_avail & AUDIO_INTERNAL_CD_IN) {
			inputs[i].name = INTERNALCD_STRING;
			inputs[i++].port = AUDIO_INTERNAL_CD_IN;
			input_avail &= ~AUDIO_INTERNAL_CD_IN;
		}
#endif

		// XXX - future device compatibility
		if ((i < MAX_INPUTS) && (input_avail != 0)) {
			inputs[i].port = input_avail;
			// XXX - advance support for Aurora CD input
			if (type == AudioDeviceCODEC) {
				inputs[i++].name = INTERNALCD_STRING;
			} else {
				// Generic name for extra input port
				inputs[i++].name = INTERNAL_STRING;
			}
		}

		for (; i < MAX_INPUTS; i++) {
			inputs[i].name = NULL;
			inputs[i].port = 0;
		}

		switch (type) {
		case AudioDeviceAMD:
			setbalance(FALSE);
			break;

		case AudioDeviceDBRI:
		case AudioDeviceCODEC:
			setbalance(TRUE);

			// If SpeakerBox with no microphone plugged in,
			// include an inactivated Microphone button
			if ((newinfo->record.avail_ports & AUDIO_LINE_IN) &&
			    !(newinfo->record.avail_ports & AUDIO_MICROPHONE)) {
				for (i = MAX_INPUTS - 1; i > 0; i--) {
					inputs[i] = inputs[i - 1];
				}
				inputs[0].name = MICROPHONE_STRING;
				inputs[0].port = AUDIO_MICROPHONE;
			}
			break;

		default:		// XXX - anything smarter?
			setbalance(TRUE);
			break;
		}
		setinports(inputs);
	}

#define	Modify(X)	if (newinfo->X != info->X)	\
			    { change->X = newinfo->X; flag = TRUE; }

	// Only change settings that changed, to avoid flashing
	Modify(record.port);
	Modify(record.avail_ports);
	Modify(record.gain);
	Modify(record.balance);
	Modify(monitor_gain);

	// If something changed, update the display
	if (flag) {
		update_display(change);
	}
	info = newinfo;		// save the latest values

	// If auto-adjust in progress, take care of it now
	if (adjusting)
		UpdateVolume();
}


// Setting new state will result in a SIGPOLL, causing
// the update routine to be entered.
// By resetting the cached values here, we guarantee that the
// value is updated in the GUI control.
// XXX - the default init value ~0 is assumed here

// Set a new record volume
void Recordctl::
SetVolume(
	double		val)
{
	Double		v = val;

	info->record.gain = ~0;
	(void) ctldev->SetRecVolume(v);
}

// Increment or decrement record volume
void Recordctl::
VolumeIncr(
	Boolean		up)
{
	info->record.gain = ~0;
	if (up)
		(void) ctldev->RecVolumeUp();
	else
		(void) ctldev->RecVolumeDown();
}

// Set a new monitor volume
void Recordctl::
SetMonitor(
	double		val)
{
	Double		v = val;

	info->monitor_gain = ~0;
	(void) ctldev->SetMonVolume(v);
}

// Increment or decrement record volume
void Recordctl::
MonitorIncr(
	Boolean		up)
{
	info->monitor_gain = ~0;
	if (up)
		(void) ctldev->MonVolumeUp();
	else
		(void) ctldev->MonVolumeDown();
}

// Set a new record balance
void Recordctl::
SetBalance(
	double		val)
{
	Double		v = val;

	info->record.balance = ~0;
	(void) ctldev->SetRecBalance(v);
}

// Set input port
void Recordctl::
SetPort(
	unsigned	port,
	Boolean		enable)
{
	AudioInfo	newinfo;

	info->record.port = ~0;

	// If this is a disable, try it
	if (!enable) {
		(void) ctldev->GetState(newinfo);	// Get current ports
		port = ~port & newinfo->record.port;	// disable
		newinfo.Clear();
	}
	newinfo->record.port = port;
	(void) ctldev->SetState(newinfo);
}


// Update the volume meter and check the input gain
void Recordctl::
UpdateVolume()
{
#define	NBUFS		(2)		// number of buffers to analyze
	int		i;
	AudioBuffer*	buf[NBUFS + 1];
	Double		frompos = AUDIO_UNKNOWN_TIME;
	Double		topos = 0.;
	Double		limit;
	AudioError	err;
	double		gain;
	double		peak;
	Boolean		clipped;
	Boolean		newgain;
	Boolean		micinput;

	// If another process is trying to record, terminate adjust
	if (info->record.waiting) {
		StopAdjust();
		return;
	}

	// Is this microphone or direct input?
	// XXX - this should be an AudioDevice class function
	micinput = (info->record.port & AUDIO_MICROPHONE);

	// If input is paused, unpause it
	if (info->record.pause)
		audiodev->Resume(ReadOnly);

	// Read until the input is drained and examine the last NBUFS buffers
	for (i = 0; i <= NBUFS; i++) {
		buf[i] = NULL;		// NBUFS+1 buffers
	}
	i = 0;
	do {
		// Allocate a temporary buffer, if necessary
		if (buf[i] == NULL) {
			buf[i] = new AudioBuffer(agc_buflen,
			    "Auto-Adjust Buffer");
			buf[i]->SetHeader(audiodev->GetHeader());
		}
		// Try to match the device read with the input buffering
		limit = agc_buflen;
		err = audiodev->Copy(buf[i], frompos, topos, limit);

		// Accumulate elapsed time
		agc_time += limit;

		// If not a whole buffer, go process the data
		if (limit < agc_buflen) {
			delete buf[i];
			buf[i] = NULL;
			if (--i < 0)		// set to previous buffer
				i = NBUFS;
			break;
		}
		if (++i > NBUFS)
			i = 0;			// loop through buffers
	} while (!err);

	// Check for read errors
	if (err && (err != AUDIO_EOF)) {
		StopAdjust();
		SetErrormsg(MGET("Audio input error"));
		for (i = 0; i <= NBUFS; i++) {
			if (buf[i] != NULL)
				delete buf[i];
		}
		return;
	}
	// Process last buffer first for led meter
	if (buf[i] != NULL) {
		(void) gainconvert.Process(buf[i],
		    AUDIO_GAIN_INSTANT | AUDIO_GAIN_WEIGHTED);
		buf[i] = NULL;
		newgain = TRUE;
	}
	// Process the rest of the buffers for weighted gain
	for (i = 0; i <= NBUFS; i++) {
		if (buf[i] != NULL) {
			(void) gainconvert.Process(buf[i], AUDIO_GAIN_WEIGHTED);
			buf[i] = NULL;
			newgain = TRUE;
		}
	}

	// If no data processed, quit now
	if (!newgain) {
		return;
	}

	// Set level meter.  If clipping, set level meter to max
	gain = gainconvert.InstantGain();
	clipped = gainconvert.Clipped();
	SetLevelmeter((clipped ? 1.0 : gain),
	    (clipped ? LEVEL_PEAK_SIGNAL : 0));


	// AGC adjustment
	gain = gainconvert.WeightedGain();
	peak = gainconvert.WeightedPeak();

	// If clipping, lower the volume and reset gain calculations
	if (clipped || (peak >= 1.) || (gain >= HiSig)) {
		if (audiodev->GetRecVolume(&info) < .3) {
			SetAdjustmsg(micinput ?
			    MGET("Speak naturally, don't overemphasize") :
			    MGET("Lower the input source level") );
		}
		VolumeIncr(FALSE);
flushgain:
		gainconvert.Flush();
		agc_time = 0.;
		settle_time = 0.;
		return;
	}

	// All other gain adjustments must be made slowly (>1 second)
	if (agc_time < AdjustTime)
		return;

	// If no perceptible signal, try setting the gain up high (once)
	if ((gain <= LoSig) && !sethigh) {
		sethigh = TRUE;
		SetVolume(1.);
		goto flushgain;
	}

	// If no signal at all, there may be something wrong
	if (peak <= NoSig) {
		SetAdjustmsg(micinput ?
		    MGET("Is the mic plugged in and switched on?") :
		    MGET("Is the input source plugged in and playing?") );

	// If the signal is really low, warn the user and boost the gain
	} else if (gain < AvgLoSig) {
		if (gain <= LoSig) {
			if (audiodev->GetRecVolume(&info) > .85) {
				SetAdjustmsg(micinput ?
				    MGET("Speak closer to the microphone") :
			    MGET("The input source level may be too low") );
			} else {
				// If the gain isn't too high, boost it double
				VolumeIncr(TRUE);
			}
		} else if (audiodev->GetRecVolume(&info) > .95) {
				SetAdjustmsg(micinput ?
				    MGET("Speak closer to the microphone") :
				    MGET("Raise the input source level") );
		}
		VolumeIncr(TRUE);
		goto flushgain;

	// If the signal too high, lower the gain a notch
	} else if (gain > AvgHiSig) {
		if (audiodev->GetRecVolume(&info) < .15) {
			SetAdjustmsg(micinput ?
			    MGET("Speak naturally, don't overemphasize") :
			    MGET("Lower the input source level") );
		}
		VolumeIncr(FALSE);
		goto flushgain;

	// If the signal is ok, make sure it remains so for a while
	} else {
		settle_time += agc_time;
		if (settle_time >= SettleTime) {
			// We're done now
			StopAdjust(TRUE);
			return;
		}
		if (settle_time >= (SettleTime / 2.))
			SetAdjustmsg(micinput ?
			    MGET("Continue speaking normally") :
			    MGET("Adjusting to direct input source") );
		else
			SetAdjustmsg(micinput ?
			    MGET("Speak at a normal level") :
			    MGET("Adjusting to direct input source") );
	}
	agc_time = 0.;
}
