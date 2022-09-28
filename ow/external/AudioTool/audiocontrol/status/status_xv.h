/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _AUDIOCONTROL_STATUS_XV_H
#define	_AUDIOCONTROL_STATUS_XV_H

#ident	"@(#)status_xv.h	1.4	93/02/25 SMI"

#include "status.h"
#include "status_ui.h"


class Statusctl_xv : public Statusctl {
private:
	// Private storage for XView status panel
	Boolean		displayed;		// true if panel is open
	Boolean		positioned;		// true if panel is positioned
	char*		default_title;		// default panel header
public:
	status_StatusPanel_objects*
			objs;			// panel objects class
	Xv_opaque	menu;			// panel menu
	Boolean		continuous;		// true if continuous update

protected:
	// Routines defined in GUI implementation code
	void		resize();

	// Methods specialized from Statusctl class
	virtual void	update_display(AudioInfo&);	// update panel

public:
			Statusctl_xv(ptr_t=NULL);	// Constructor
	virtual		~Statusctl_xv();		// Destructor

	virtual void	Unshow(int);		// Internal routine
	virtual void	set_timer(Boolean);	// enable/disable timer

	// Methods specialized from Audioctl class
	virtual int	Setdevice(char*);	// set device name
	virtual int	Setpositioned(int);	// set positioned flag
	virtual void	Show();			// Display the panel
	virtual void	Unshow();		// Close the panel
	virtual ptr_t	Gethandle();		// Return panel handle
	virtual void	Setwidth(int);		// set panel width hint
};

#endif /* !_AUDIOCONTROL_STATUS_XV_H */
