#ifndef	_MODEMGR_H
#define	_MODEMGR_H

#ident "@(#)modemgr.h	1.22 96/11/15 Copyright 1990 Sun Microsystems, Inc."

#include "uimgr.h"
#include "xview.h"
#include <xview/notify.h>


// Forward references.
//
class	NOTIFY;


// Navigator UI modes.
//
typedef	enum {
	MODE_TOC	= 0,
	MODE_SEARCH	= 1,
	MODE_BOOKMARK	= 2,
	MODE_NONE	= 99
} UIMODE;



// Different types of MODEMGR events.
// Note that the numeric value of each of these events must be unique
// over all Navigator events, which is why we assign a specific
// value to them (XXX but we should find a cleaner method than this).
//
typedef enum {
	MM_MODE_SELECT_EVENT	= 101,
	MM_VIEW_EVENT		= 104,
	MM_BROWSE_EVENT		= 107,
	MM_BOOKMARK_EVENT	= 108,
	MM_PANEL_RESIZE_EVENT	= 110
} MM_EVENT;


class	MODEMGR {

    private:

	// XView panel containing widgets common to all modes.
	//
	static Xv_opaque	main_panel;

	// XView widgets common to all modes.
	// 
	static Xv_opaque	main_view_button;
	static Xv_opaque	main_bookmark_button;
	static Xv_opaque	main_browse_button;
	static Xv_opaque	main_mode_choice;

	// Are common widgets initialized?
	//
	static BOOL		main_is_inited;

	// Event handler and accompanying callback argument for
	// MODEMGR events.
	//
	EVENT_HANDLER		event_handler;
	caddr_t			event_client_data;

	// Initialize common widgets.
	//
	void			InitMainWidgets(Xv_opaque frame);

	// Handlers for various events.
	//
	static void		WidgetEvent(Panel_item, Event *);
	static void		MenuEvent(Menu, Menu_item);
	static Notify_value	PanelEvent(	Xv_opaque panel,
						Event *event,
						Notify_arg arg,
						Notify_event_type type);

    protected:

	// XView panel containing mode-specific widgets.
	//
	Xv_opaque	mode_panel;

	// Message handler.
	//
	NOTIFY		*notify;

	// current state of this object.
	//
	OBJECT_STATE	objstate;

	// Virtual event handler for handling mode-specific MODEMGR events.
	// 
	virtual void	EventHandler(int event, caddr_t event_obj) = 0;

	// Default event handler for handling MODEMGR events.
	// 
	void		DefaultEventHandler(int event, caddr_t event_obj);

	// MODEMGR constructor.
	// This is "protected" because we don't want anyone other than
	// derived classes to create new MODEMGRs.
	//
	MODEMGR(Xv_opaque frame, NOTIFY *notify);


    public:

	virtual		~MODEMGR();

	// Display widgets for the current mode.
	//
	virtual void	Show(BOOL show) = 0;

	// Get DOCNAME of currently selected document.
	//
	virtual const DOCNAME	&GetSelection() const = 0;

	// Set/get current UI mode.
	//
	void		SetMode(int mode);
	int		GetMode();

	// Register event handler for MODEMGR events.
	//
	void		SetEventHandler(EVENT_HANDLER func, caddr_t clnt)
			{ event_handler = func; event_client_data = clnt; }
};

#endif	_MODEMGR_H
