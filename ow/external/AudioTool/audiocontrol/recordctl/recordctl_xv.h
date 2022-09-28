/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _AUDIOCONTROL_RECORDCTL_XV_H
#define	_AUDIOCONTROL_RECORDCTL_XV_H

#ident	"@(#)recordctl_xv.h	1.9	93/06/06 SMI"

#include "recordctl.h"
#include "recordctl_ui.h"


class Recordctl_xv : public Recordctl {
private:
	// Private storage for XView record control panel
	Boolean		displayed;		// true if panel is open
	Boolean		balance;		// true if balance control
	Boolean		positioned;		// true if position set
	Xv_opaque	inports[MAX_INPUTS];	// input port items
	double		meter_steps;		// number of level meter leds
public:
	recordctl_RecordctlPanel_objects*
			objs;			// panel objects class
	Xv_opaque	menu;			// panel menu
	double		max_gain;		// panel gain range
	unsigned int	last_volume;		// last volume set
	unsigned int	last_monitor;		// last monitor volume set
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
	void		setadjustbutton();
	// bugid 1129190
	int		maxbuttonwidth();

	// Methods specialized from Recordctl class
	virtual void	update_display(AudioInfo&);	// update panel
	virtual void	setbalance(Boolean);		// enable/disable
	virtual void	setinports(DevicePort*);	// set ports

	virtual void	SetAdjustmsg(char* = NULL);	// set msg string
	virtual void	SetErrormsg(char* = NULL);	// display error msg
	virtual void	SetLevelmeter(double, int = 0);	// set led meter
	virtual void	StartAdjust();			// start auto-adjust

public:
			Recordctl_xv(ptr_t=NULL);	// Constructor
	virtual		~Recordctl_xv();	// Destructor

	virtual void	Unshow(int);		// Internal routine
	virtual void	startstop_button();	// Auto-Adjust button toggle
	virtual void	StopAdjust(int=0);		// stop auto-adjust

	// Methods specialized from Audioctl class
	virtual int	Setpositioned(int);	// set positioned flag
	virtual void	Show();			// Display the panel
	virtual void	Unshow();		// Close the panel
	virtual ptr_t	Gethandle();		// Return panel handle
	virtual void	Setwidth(int);		// set panel width hint
};

#endif /* !_AUDIOCONTROL_RECORDCTL_XV_H */
