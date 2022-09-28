#ifndef	_XVPANEL_H_
#define	_XVPANEL_H_

#ident "@(#)xvpanel.h	1.38 11/15/96 Copyright 1989 Sun Microsystems, Inc."

#include "common.h"
#include <xview/panel.h>

typedef enum {
	FOLLOW_LINK		= 1,
	PAGE_INFO,
	PARTIAL_FULL_PAGE,
	STANDARD_MAG,
	CUSTOM_MAG
} ViewMenuValue;

class XVPANEL {

	private:

	// XView panel widget
	Panel		panel;

	// XView widgets on the panel
	Menu_item	followLinkItem;
	Panel_item	goBkButton;
	Panel_item	printButton;
	Panel_item	pgBwdButton;
	Panel_item	pgFwdButton;
	Panel_item	viewButton;
	Menu		viewMenu;

	// Client of this object can register an event proc to handle
	// events that this object cannot handle
	//
	ViewerEventProc	defaultEventProc;
	void	       *cdata;

	// miscellaneous stuff
	//
	ViewerType	viewerType;

	static Xv_opaque _dataKey;


	// Private methods
	//
	void		CreateButtons(const ViewerType vtype);
	void		CustomMag();
	void		LayOutPanel();

	// XView call backs for the various widgets on the panel
	//
	static int	GoBkButtonNotifyProc(Panel_item button, Event *);
	static void	PgButtonNotifyProc(Panel_item button, Event *);
	static int	PrintButtonNotifyProc(Panel_item button, Event *);
	static void	ViewMenuActionProc(Menu, Menu_item);
	static void	EventProc(const Xv_window	win,
				  Event *const		event,
				  const Notify_arg	/* unused */);

	public:

	XVPANEL();
	~XVPANEL();

	STATUS		Init(const Frame	base_frame,
			     const ViewerType	vtype,
			     ERRSTK		&err);

	void		DisableGoBack(BOOL bool)
	{
		(void) xv_set(goBkButton, PANEL_INACTIVE, bool, XV_NULL);
	}
	void		DisablePgBwdButton(BOOL bool)
	{
		(void) xv_set(pgBwdButton, PANEL_INACTIVE, bool, XV_NULL);
	}
	void		DisablePgFwdButton(BOOL bool)
	{
		(void) xv_set(pgFwdButton, PANEL_INACTIVE, bool, XV_NULL);
	}

	void		EnableFollowLinkItem(BOOL bool);

	int		Height() const
	{
		assert(panel);

		return((int) xv_get(panel, XV_HEIGHT));
	}

	int		MinWidth();

	// Set client's event handler
	void		SetEventProc(const ViewerEventProc	ptrToFunc,
				     const void		       *client_data)
	{
		defaultEventProc	= ptrToFunc;
		cdata			= (void *) client_data;
	}

	void		SetWidth(int);
};
#endif	/* _XVPANEL_H_ */
