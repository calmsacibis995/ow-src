#ident "@(#)xvsubwin.cc	1.30 11/02/94 Copyright 1990 Sun Microsystems, Inc."

#include "spothelp.h"
#include "ds_popup.h"
#include "xvsubwin.h"

#ifndef SVR4
#include <strings.h>
#endif

#include <math.h>

inline float	ToFraction(const int m)
{
	return((float) m / 100.0);
}

inline Xv_opaque GetItemValue(const Panel_item item)
{
	return(xv_get(item, PANEL_VALUE));
}

inline Xv_opaque SetItemValue(const Xv_opaque obj, Xv_opaque value)
{
	return(xv_set(obj, PANEL_VALUE, value, XV_NULL));
}

// Functions called
#ifdef DEBUG
extern void		PrintRect(char *, Xv_opaque);
#endif /* DEBUG */

int
MAGWIN::ApplyNotifyProc(Panel_item button, Event *)
{
	MAGWIN	       *ptr = (MAGWIN *) xv_get(button, PANEL_CLIENT_DATA);
	float		magFraction;

	DbgFunc("MAGWIN::ApplyChanges: entered" << endl);
	assert(notify);
	(void) xv_set(ptr->frame, XV_SHOW, FALSE, XV_NULL);

	notify->Busy(gettext("Applying changes ..."));

	ptr->sliderValue	= (int) GetItemValue(ptr->magItem);
	magFraction		= ToFraction(ptr->sliderValue);

	ptr->defaultEventProc(VE__MAGNIFY,
			      (const void *) &magFraction,
			      ptr->cdata);

	SetItemValue(ptr->magItem, ptr->sliderValue);

	notify->Done();

	return(XV_OK);
}

STATUS
MAGWIN::CreatePanelItems(ERRSTK &err)
{
	const char	*flabel	= gettext("Custom Magnification");
	const char	*plabel	= gettext("Magnification Percentage:");

	int		hght;
	int		wdth;
	STATUS		status	= STATUS_OK;

	assert(frame && panel);

	(void)	xv_set(frame,				
		       FRAME_LABEL,			flabel,
		       FRAME_SHOW_FOOTER,		FALSE,
		       FRAME_SHOW_LABEL,		TRUE,
		       WIN_CLIENT_DATA,			(caddr_t) this,
		       FRAME_PROPS_PUSHPIN_IN,		FALSE,
		       XV_NULL);

	(void)	xv_set(panel,
		       PANEL_LAYOUT,			PANEL_VERTICAL,
		       XV_NULL);

	magItem	= (Panel_item)
		xv_create(panel,			PANEL_SLIDER,
			  PANEL_SLIDER_WIDTH,		150,
			  PANEL_LABEL_BOLD,		TRUE,
			  PANEL_LABEL_STRING,		plabel,
			  PANEL_LAYOUT,			PANEL_HORIZONTAL,
			  PANEL_DIRECTION,		PANEL_HORIZONTAL,
			  PANEL_SLIDER_END_BOXES,	FALSE,
			  PANEL_SHOW_RANGE,		TRUE,
			  PANEL_SHOW_VALUE,		TRUE,
			  PANEL_READ_ONLY,		TRUE,
			  PANEL_MIN_VALUE,		25,
			  PANEL_MAX_VALUE,		200,
			  PANEL_TICKS,			10,
			  XV_HELP_DATA,			MAGNIFY_SLIDER_HELP,
			  XV_NULL);
			  
	(void)	xv_set(panel, PANEL_LAYOUT, PANEL_VERTICAL, XV_NULL);

	applyButton	= (Panel_item)
		xv_create(panel,			PANEL_BUTTON,
			  PANEL_CLIENT_DATA,		(caddr_t) this,
			  PANEL_LABEL_STRING,		gettext("Apply"),
			  PANEL_LAYOUT,			PANEL_HORIZONTAL,
			  PANEL_NOTIFY_PROC,	       MAGWIN::ApplyNotifyProc,
			  XV_HELP_DATA,			APPLY_BUTTON_HELP,
			  XV_NULL);

	(void)	xv_set(panel, PANEL_LAYOUT, PANEL_HORIZONTAL, XV_NULL);

	resetButton =
		xv_create(panel,			PANEL_BUTTON,
			  PANEL_CLIENT_DATA,		(caddr_t) this,
			  PANEL_LABEL_STRING,		gettext("Reset"),
			  PANEL_LAYOUT,			PANEL_HORIZONTAL,
			  PANEL_NOTIFY_PROC,	       MAGWIN::ResetNotifyProc,
			  XV_HELP_DATA,			RESET_BUTTON_HELP,
			  XV_NULL);

	if (!(magItem && applyButton && resetButton)) {
		DbgHigh("MAGWIN::Init: " <<
			"creation of XView objects failed" << endl);
		err.Init(gettext("Error creating items on custom magnification popup"));
		status = STATUS_FAILED;
	}
	else {
		wdth	= ((int) xv_get(magItem, XV_WIDTH) +
			   (2 * (int)xv_get(panel, PANEL_ITEM_X_GAP)) +
			   (2 * PANEL_ITEM_X_START));

		hght	= ((int) xv_get(magItem, XV_HEIGHT) +
			   (int) xv_get(applyButton, XV_HEIGHT) +
			   (2 * (int)xv_get(panel, PANEL_ITEM_Y_GAP)) +
			   (2 * PANEL_ITEM_Y_START));

		(void) xv_set(panel,
			      XV_WIDTH,		wdth,
			      XV_HEIGHT,	hght,
			      XV_NULL);

		window_fit(frame);

		LayOutPanel();

		objstate.MarkReady();
	}

	return(status);
}

STATUS
MAGWIN::Init(const Frame baseFrame, ERRSTK &err)
{
	STATUS		status;
	int set_input_method = 0; 

	DbgFunc("MAGWIN::MAGWIN: entered" << endl);
	assert(objstate.IsNotReady());

	if ((status = SUBWIN::Init(baseFrame, set_input_method, err)) == STATUS_OK) {
		status = CreatePanelItems(err);
	}

	if (status == STATUS_OK) {
		ds_position_popup(baseFrame, frame, DS_POPUP_LOR);
	}
	else {
	err.Init(gettext("Could not create custom magnification popup"));
	}

	return(status);
}

void
MAGWIN::LayOutPanel()
{
	const int	panelWdth = (int) xv_get(panel, XV_WIDTH, XV_NULL);
	int		xgap;
	register int	xloc;
	int		ygap;
	register int	yloc;

	DbgFunc("MAGWIN::LayOutPanel: entered" << endl);
	assert(panel && magItem && applyButton && resetButton);
	assert(objstate.IsNotReady());

	xgap	= (int) xv_get(panel, PANEL_ITEM_X_GAP);
	ygap	= (int) xv_get(panel, PANEL_ITEM_Y_GAP);

	// Locate the magItem slider object
	xloc	= panelWdth -
		((int)xv_get(magItem, XV_WIDTH) + PANEL_ITEM_X_START + xgap);

	yloc	= PANEL_ITEM_Y_START;

	(void) xv_set(magItem, XV_X, xloc, XV_Y, yloc, XV_NULL);

	// Locate the applyButton
	xloc	= rint((panelWdth/2.0) -
		       ((int)xv_get(applyButton, XV_WIDTH) + xgap));

	yloc	+= ((int) xv_get(magItem, XV_HEIGHT) + ygap);

	(void) xv_set(applyButton, XV_X, xloc, XV_Y, yloc, XV_NULL);



	// Locate the applyButton
	xloc	= rint((panelWdth/2.0) + xgap);
	(void) xv_set(resetButton, XV_X, xloc, XV_Y, yloc, XV_NULL);

	return;
}

int
MAGWIN::ResetNotifyProc(Panel_item button, Event *)
{
	MAGWIN	       *ptr = (MAGWIN *) xv_get(button, PANEL_CLIENT_DATA);

	DbgFunc("MAGWIN::ResetNotifyProc: entered" << endl);
	assert(ptr->magItem);

	(void) xv_set(ptr->magItem,
		      PANEL_VALUE,	ptr->sliderValue,
		      XV_NULL);

	return(XV_OK);
}

void
MAGWIN::Show(const int	minMagPercent,
	     const int	magPercent)
{
	DbgFunc("MAGWIN::ShowWin: entered" << endl);
	assert(objstate.IsReady());

	sliderValue	= (magPercent < 0) ? 0 : minMagPercent;

	(void) xv_set(magItem,
		      PANEL_VALUE,		magPercent,
		      PANEL_MIN_VALUE,		minMagPercent,
		      XV_NULL);

	SUBWIN::Show();
}
