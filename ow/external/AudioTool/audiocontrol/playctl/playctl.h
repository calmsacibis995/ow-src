/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _PLAYCTL_H
#define	_PLAYCTL_H

#ident	"@(#)playctl.h	1.5	92/10/23 SMI"

#include "audioctl.h"


const int	MAX_OUTPUTS = 3;	// Maximum number of output ports


// Play Control Panel base class (toolkit-independent)

class Playctl : public Audioctl {
private:
	// Output port name strings
	char*	SPEAKER_STRING;
	char*	HEADPHONE_STRING;
	char*	LINEOUT_STRING;

	// Private storage for generic play control
	Boolean		output_excl;		// true if exclusive output

protected:
	Boolean		output_exclusive();		// true, if excl

	// Methods that must be specialized
	virtual void	update_display(AudioInfo&) = 0;	// update panel
	virtual void	setbalance(Boolean) = 0;	// enable/disable
	virtual void	setoutports(DevicePort*) = 0;	// set ports

public:
	Playctl(ptr_t parent = NULL);			// Constructor

	// Playctl methods for device control
	virtual void	SetVolume(double);		// set volume
	virtual void	VolumeIncr(Boolean);		// incr/decr volume
	virtual void	SetBalance(double);		// set balance
	virtual void	SetMute(Boolean);		// set/clr mute
	virtual void	SetPort(unsigned, Boolean);	// change outport

	// Public methods
	virtual void	Update();
};

// Global routine to instantiate a toolkit-dependent panel
extern Playctl*		Create_Playctl(char*, ptr_t);


#endif /* !_PLAYCTL_H */
