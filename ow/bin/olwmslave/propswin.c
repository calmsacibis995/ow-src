/*
 *      (c) Copyright 1989, 1990 Sun Microsystems, Inc. Sun design patents
 *      pending in the U.S. and foreign countries. See LEGAL_NOTICE
 *      file for terms of the license.
 */
/* ----------------------------------------------------------------------
 *	propswin.c
 * ----------------------------------------------------------------------*/
#ifndef line
#ifdef sccs
static char     sccsid[] = "@(#) propswin.c 1.6 91/10/21 Sun Micro";
#endif
#endif

#include <xview/xview.h>
#include <xview/panel.h>
#include "olwmslave.h"
#include "mem.h"

/* ----------------------------------------------------------------------
 *      Window Properties structure definition
 * ----------------------------------------------------------------------*/
typedef struct {
	int		wpScreenNo;
	Frame		wpFrame;
} WindowProps;

/* ----------------------------------------------------------------------
 *      Local Data
 * ----------------------------------------------------------------------*/

static WindowProps	*winPropsList = 0; 	/* dynamic array of WindowProps
					     	 * one for each server */
#define WinProps(nscr)	(&winPropsList[(nscr)]);/* convenience macros to access
						 * list as array */

/* ----------------------------------------------------------------------
 *      Local Forward Declarations
 * ----------------------------------------------------------------------*/
int		ShowWindowProps();
void		InitWindowProps();
Bool		CreateWindowProps();
Notify_value	DestroyWindowProps();
void		ApplyProps();
void		ResetProps();

/* ----------------------------------------------------------------------
 *      ShowWindowProps
 * ----------------------------------------------------------------------*/
int
ShowWindowProps(nscreen)
	int		nscreen;
{
	WindowProps	*wp;

	if (!ScreenUsed(nscreen)) {
		return False;
	}
	if (!winPropsList) {
		InitWindowProps();
	}
	wp = WinProps(nscreen);
	if (!CreateWindowProps(wp)) {
		return False;
	}
	xv_set(wp->wpFrame,
		XV_SHOW,	TRUE,
		WIN_FRONT,
		NULL);

	return True;
}

/* ----------------------------------------------------------------------
 *      InitWindowProps
 * ----------------------------------------------------------------------*/
static void
InitWindowProps()
{
	WindowProps	*wp;
	int		i;

	winPropsList = (WindowProps *)MemAlloc(
		sizeof(WindowProps)*NumScreens());
	for (i=0; i<NumScreens(); i++) {
		wp = WinProps(i);
		wp->wpScreenNo = i;
		wp->wpFrame = (Frame)NULL;
	}
}

/* ----------------------------------------------------------------------
 *      CreateWindowProps
 * ----------------------------------------------------------------------*/
static Bool
CreateWindowProps(wp)
	WindowProps	*wp;
{
	Panel		wpPanel;
#ifdef OW_I18N_L4
	wchar_t *wpwc = mbstowcsdup(LOCALIZE("Window Properties"));
	wchar_t *wpniwc = mbstowcsdup(LOCALIZE("Window Properties Not Implemented"));
	wchar_t *apwc = mbstowcsdup(LOCALIZE("Apply"));
	wchar_t *rewc = mbstowcsdup(LOCALIZE("Reset"));
#endif

	wp->wpFrame = (Frame)xv_create(XVRoot(wp->wpScreenNo),FRAME_PROPS,
		WIN_ROWS,		1,
		WIN_COLUMNS,		33,
		FRAME_CMD_PUSHPIN_IN,	TRUE,
#ifdef OW_I18N_L4
		FRAME_LABEL_WCS,	wpwc,
#else
		FRAME_LABEL,		LOCALIZE("Window Properties"),
#endif
		XV_KEY_DATA,		winPropsList, wp,
		XV_SHOW,		FALSE,
		NULL);
	notify_interpose_destroy_func(wp->wpFrame,DestroyWindowProps);

	wpPanel = (Panel)xv_create(wp->wpFrame,PANEL,
		WIN_ROWS,		1,
		NULL);
	(void)xv_create(wpPanel,PANEL_MESSAGE,
#ifdef OW_I18N_L4
		PANEL_LABEL_STRING_WCS, wpniwc,
#else
		PANEL_LABEL_STRING, 	LOCALIZE("Window Properties Not Implemented"),
#endif
		NULL);
#ifdef notdef
	(void)xv_create(wpPanel,PANEL_BUTTON,
#ifdef OW_I18N_L4
		PANEL_LABEL_STRING_WCS,	apwc,
#else
		PANEL_LABEL_STRING,	LOCALIZE("Apply"),
#endif
		PANEL_NOTIFY_PROC,	ApplyProps,
		NULL);
	(void)xv_create(wpPanel,PANEL_BUTTON,
#ifdef OW_I18N_L4
		PANEL_LABEL_STRING_WCS,	rewc,
#else
		PANEL_LABEL_STRING,	LOCALIZE("Reset"),
#endif
		PANEL_NOTIFY_PROC,	ResetProps,
		NULL);
#endif
#ifdef OW_I18N_L4
	free(wpwc);
	free(wpniwc);
	free(apwc);
	free(rewc);
#endif
	window_fit(wpPanel);
	window_fit(wp->wpFrame);

	return True;
}

/* ----------------------------------------------------------------------
 *      DestroyWindowProps
 * ----------------------------------------------------------------------*/
static Notify_value
DestroyWindowProps(client,status)
	Notify_client	client;
	Destroy_status	status;
{
	WindowProps	*wp;

	if (status != DESTROY_CLEANUP) {
		wp = (WindowProps *)xv_get(client,XV_KEY_DATA,winPropsList);
		if (wp) {
			wp->wpFrame = (Frame)NULL;
		}
	}
	return (notify_next_destroy_func(client,status));
}

#ifdef notdef
/* ----------------------------------------------------------------------
 *      ApplyProps
 * ----------------------------------------------------------------------*/
static void
ApplyProps(item,event)
	Panel_item	item;
	Event		*event;
{
}

/* ----------------------------------------------------------------------
 *      ResetProps
 * ----------------------------------------------------------------------*/
static void
ResetProps(item,event)
	Panel_item	item;
	Event		*event;
{
}
#endif
