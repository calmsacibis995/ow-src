/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _STATUS_H
#define	_STATUS_H

#ident	"@(#)status.h	1.2	92/10/22 SMI"

#include "audioctl.h"


// Status Panel base class (toolkit-independent)

class Statusctl : public Audioctl {
private:

protected:
	// Methods that must be specialized
	virtual void	update_display(AudioInfo&) = 0;	// update panel

public:
			Statusctl(ptr_t=NULL);		// Constructor

	// Public methods
	virtual void	Update();
};

// Global routine to instantiate a toolkit-dependent panel
extern Statusctl*	Create_Statusctl(char*, ptr_t);


#endif /* !_STATUS_H */
