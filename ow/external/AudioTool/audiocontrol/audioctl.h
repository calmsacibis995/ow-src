/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _AUDIOCTL_H
#define	_AUDIOCTL_H

#ident	"@(#)audioctl.h	1.6	93/02/25 SMI"

#include <multimedia/AudioDevicectl.h>
#include "audiocontrol.h"


class DevicePort {
public:
	char*			name;		// name string
	unsigned		port;		// port bits
};


// Control Panel virtual base class

class Audioctl {
protected:
	ptr_t		parent;			// parent panel
	AudioDevicectl*	ctldev;			// control device
	AudioDeviceType	type;			// device type
	AudioInfo	info;			// cached device state
	int		min_width;		// minimum panel width

	virtual int	newdevice(char*);		// set device name

public:
			Audioctl(ptr_t= NULL);		// Constructor
	virtual		~Audioctl();			// Destructor

	// Public interface
	virtual int	Setdevice(char*);		// set device name
	virtual int	Setpositioned(int) = 0;		// set positioned state
	virtual char*	Getdevice();			// get device name
	virtual void	Update() = 0;			// update panel values
	virtual void	Show() = 0;			// display the panel
	virtual void	Unshow() = 0;			// close the panel
	virtual ptr_t	Gethandle() = 0;		// return handle
	virtual int	Getwidth();			// return panel width
	virtual void	Setwidth(int) = 0;		// set panel width hint
};

#endif /* !_AUDIOCTL */
