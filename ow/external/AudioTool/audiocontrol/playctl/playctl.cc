/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)playctl.cc	1.19	96/02/20 SMI"

#include <stdio.h>
#include <unistd.h>

#include "playctl.h"



// Play Control Panel implementation


// Constructor
Playctl::
Playctl(ptr_t parent_local):
	Audioctl(parent_local)
{
	// Init output port names (extra spaces assume variable width fonts)
	SPEAKER_STRING = MGET("Speaker");
	HEADPHONE_STRING = MGET("Headphone");
	LINEOUT_STRING = MGET("Line Out");
}

// Get the latest device state and update the panels appropriately.
void Playctl::
Update()
{
	AudioInfo	newinfo;
	AudioInfo	change;
	Boolean		flag;
	int		i;
	DevicePort	outputs[MAX_OUTPUTS];

	if (ctldev == NULL) {
		info.Clear();
		return;
	}

	// Get the current device state
	(void) ctldev->GetState(newinfo);

	// XXX - patch available ports for 4.x AMD devices
	if ((type == AudioDeviceAMD) &&
	    ((newinfo->play.avail_ports == 0) ||
	    (newinfo->play.avail_ports == 1) ||
	    (newinfo->play.avail_ports == (unsigned)~0))) {
		newinfo->play.avail_ports = AUDIO_SPEAKER | AUDIO_HEADPHONE;
	}


	// If the device type may have changed, get the new type
	// and update the panel appearance to match
	if (type == (AudioDeviceType) -1) {
		type = ctldev->GetDeviceType();

		// XXX - backwards compatability for 4.x AMD devices
		if ((type == AudioDeviceAMD) &&
		    ((newinfo->play.avail_ports == 0) ||
		    (newinfo->play.avail_ports == 1) ||
		    (newinfo->play.avail_ports == (unsigned)~0))) {
			newinfo->play.avail_ports =
			    AUDIO_SPEAKER | AUDIO_HEADPHONE;
		}

		// Initialize the output port names
		i = 0;
		if (newinfo->play.avail_ports & AUDIO_SPEAKER) {
			outputs[i].name = SPEAKER_STRING;
			outputs[i++].port = AUDIO_SPEAKER;
		}
		if (newinfo->play.avail_ports & AUDIO_HEADPHONE) {
			outputs[i].name = HEADPHONE_STRING;
			outputs[i++].port = AUDIO_HEADPHONE;
		}
		if (newinfo->play.avail_ports & AUDIO_LINE_OUT) {
			outputs[i].name = LINEOUT_STRING;
			outputs[i++].port = AUDIO_LINE_OUT;
		}
		for (; i < MAX_OUTPUTS; i++) {
			outputs[i].name = NULL;
			outputs[i].port = 0;
		}

		switch (type) {
		case AudioDeviceAMD:
			setbalance(FALSE);
			output_excl = TRUE;
			break;

		case AudioDeviceDBRI:
		case AudioDeviceCODEC:
			setbalance(TRUE);
			output_excl = FALSE;

			// If SpeakerBox with no headphones plugged in,
			// include an inactivated Headphone button
			if ((newinfo->play.avail_ports & AUDIO_SPEAKER) &&
			    (newinfo->play.avail_ports & AUDIO_LINE_OUT) &&
			    !(newinfo->play.avail_ports & AUDIO_HEADPHONE)) {
				outputs[2] = outputs[1];
				outputs[1].name = HEADPHONE_STRING;
				outputs[1].port = AUDIO_HEADPHONE;
			}
			break;

		default:		// XXX - anything smarter?
			setbalance(TRUE);
			output_excl = FALSE;
			break;
		}
		setoutports(outputs);
	}

#define	Modify(X)	if (newinfo->X != info->X)	\
			    { change->X = newinfo->X; flag = TRUE; }

	// Only change settings that changed, to avoid flashing
	Modify(play.port);
	Modify(play.avail_ports);
	Modify(play.gain);
	Modify(play.balance);
	Modify(output_muted);

	/* If something changed */
	if (flag) {
		info = newinfo;		/* save latest values */
		update_display(change);
	}
}

// Return TRUE if only a single output port may be active at a time
Boolean Playctl::
output_exclusive()
{
	return (output_excl);
}


// Setting new state will result in a SIGPOLL, causing
// the update routine to be entered.
// By resetting the cached values here, we guarantee that the
// value is updated in the GUI control.
// XXX - the default init value ~0 is assumed here

// Set a new play volume
void Playctl::
SetVolume(
	double		val)
{
	Double		v = val;

	info->play.gain = ~0;
	(void) ctldev->SetPlayVolume(v);
}

// Increment or decrement play volume
void Playctl::
VolumeIncr(
	Boolean		up)
{
	info->play.gain = ~0;
	if (up)
		(void) ctldev->PlayVolumeUp();
	else
		(void) ctldev->PlayVolumeDown();
}

// Set a new play balance
void Playctl::
SetBalance(
	double		val)
{
	Double		v = val;

	info->play.balance = ~0;
	(void) ctldev->SetPlayBalance(v);
}

// Toggle the mute state
void Playctl::
SetMute(
	Boolean		set)
{
	AudioInfo	newinfo;

	info->output_muted = ~0;
	newinfo->output_muted = set;
	(void) ctldev->SetState(newinfo);
}

// Set output port
void Playctl::
SetPort(
	unsigned	port,
	Boolean		enable)
{
	AudioInfo	newinfo;

	info->play.port = ~0;

	// Set new port value according to whether or not outputs are exclusive
	if (output_exclusive()) {
		// XXX - If exclusive output, this better not be a disable!!
		if (!enable) {
			Update();
			return;
		}
	} else {
		(void) ctldev->GetState(newinfo);	// Get current ports
		if (enable)
			port |= newinfo->play.port;	// enable this one
		else
			port = ~port & newinfo->play.port;	// disable
		newinfo.Clear();
	}
	newinfo->play.port = port;
	(void) ctldev->SetState(newinfo);
}
