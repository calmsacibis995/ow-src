#ifndef	_XVSUBWIN_H_
#define	_XVSUBWIN_H_

#ident "@(#)xvsubwin.h	1.16 07/06/93 Copyright 1989 Sun Microsystems, Inc."

#include "common.h"
#include <xview/panel.h>

// Forward declarations

class	SUBWIN {
protected:
	Frame		frame;
	Panel		panel;

	// Client of this object can register an event proc to handle
	// events that this object cannot handle
	//
	ViewerEventProc	defaultEventProc;
	void	       *cdata;

	SUBWIN() :
	cdata			(NULL),
	defaultEventProc	(NULL),
	frame			(XV_NULL),
	panel			(XV_NULL)
	{ }

	STATUS		Init(const Frame parent, int use_im, ERRSTK &err)
	{
		STATUS	status = STATUS_OK;

		frame = (Frame)
			xv_create(parent,		FRAME_PROPS,
				  FRAME_NO_CONFIRM,	TRUE,
				  WIN_USE_IM,		(use_im == 1) ? TRUE : FALSE,
				  XV_SHOW,		FALSE,
				  XV_NULL);

		panel = (Panel) xv_get(frame, FRAME_PROPS_PANEL);

		if (!frame || !panel) {
			err.Init(gettext("Could not create custom magnification popup's frame and/or panel"));
			status = STATUS_FAILED;
		}

		return(status);
	}

	virtual ~SUBWIN()
	{ }

public:
	void		SetEventProc(const ViewerEventProc	ptrToFunc,
				     const void			*client_data)
	{
		defaultEventProc	= ptrToFunc;
		cdata			= (void *) client_data;
	}

	void		Show()
	{
		(void) xv_set(frame,
			      XV_SHOW,			TRUE,
			      XV_NULL);

	}
};

class	MAGWIN : public SUBWIN {
private:
	// XView items
	Panel_item	applyButton;
	Panel_item	magItem;
	OBJECT_STATE	objstate;
	Panel_item	resetButton;
	u_int		sliderValue;

	// Private functions
	static int	ApplyNotifyProc(Panel_item, Event *);
	STATUS		CreatePanelItems(ERRSTK &err);
	void		LayOutPanel();
	static int	ResetNotifyProc(Panel_item, Event *);


public:
	MAGWIN() : sliderValue(100) { }

	STATUS		Init(const Frame	base,
			     ERRSTK		&err);

	BOOL		IsReady() const
	{
		return(objstate.IsReady());
	}

	void		Show(const int	minMagPercent,
			     const int	magPercent);
};
#endif	/* _XVSUBWIN_H_ */
