/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _AUDIOCONTROL_PLAYCTL_XV_H
#define	_AUDIOCONTROL_PLAYCTL_XV_H

#ident	"@(#)playctl_xv.h	1.8	93/02/25 SMI"

#include "playctl.h"
#include "playctl_ui.h"


class Playctl_xv : public Playctl {
private:
	// Private storage for XView play control panel
	Boolean		iconic;			// false if panel is open
	Boolean		displayed;		// true if panel is mapped
	Boolean		positioned;		// is it positioned yet?
	Boolean		balance;		// true if balance control
	Xv_opaque	outports[MAX_OUTPUTS];	// output port items
public:
	playctl_PlayctlPanel_objects*
			objs;			// panel objects class
	Xv_opaque	menu;			// panel menu
	double		max_gain;		// panel gain range
	unsigned int	last_volume;		// last volume set
	unsigned int	max_balance;		// panel balance range
	unsigned int	last_balance;		// last balance set
	Boolean		dragbalance;		// TRUE during balance drag
	unsigned int	dragvalue;		// last balance during drag

protected:
	// Routines defined in GUI implementation code
	void		destroy_object(Xv_opaque&);
	void		destroy_balance();
	void		create_balance();
	void		resize();
	int		unscale_gain(unsigned int);

	// Methods specialized from Playctl class
	virtual void	update_display(AudioInfo&);	// update panel
	virtual void	setbalance(Boolean);		// enable/disable
	virtual void	setoutports(DevicePort*);	// set ports

public:
			Playctl_xv(ptr_t parent = NULL); // Constructor
	virtual		~Playctl_xv();		// Destructor

	// Methods specialized from Audioctl class
	virtual int	Setpositioned(int);	// set positioned state
	virtual void	Show();			// Display the panel
	virtual void	Unshow();		// Close the panel
	virtual ptr_t	Gethandle();		// Return panel handle
	virtual void	Setwidth(int);		// set panel width hint

	void		Iconify();		// iconify the panel
};

#endif /* !_AUDIOCONTROL_PLAYCTL_XV_H */
