#ifndef	_XOL_OLDNDVCX_H
#define	_XOL_OLDNDVCX_H

#pragma	ident	"@(#)OlDnDVCX.h	302.2	92/04/02 include/Xol SMI"	/* OLIT	*/

/*
 *	Copyright (C) 1986,1991  Sun Microsystems, Inc
 *			All rights reserved.
 *		Notice of copyright on this source code
 *		product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *	Sun Microsystems, Inc., 2550 Garcia Avenue,
 *	Mountain View, California 94043.
 *
 */


#include <Xol/OpenLook.h>

#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


extern	Atom	_SUN_AVAILABLE_TYPES;

extern	Atom	_SUN_LOAD;
extern	Atom	_SUN_DATA_LABEL;
extern	Atom	_SUN_FILE_HOST_NAME;

extern	Atom	_SUN_ENUMERATION_COUNT;
extern	Atom	_SUN_ENUMERATION_ITEM;

extern	Atom	_SUN_ALTERNATE_TRANSPORT_METHODS;
extern	Atom	_SUN_LENGTH_TYPE;
extern	Atom	_SUN_ATM_TOOL_TALK;
extern	Atom	_SUN_ATM_FILE_NAME;

typedef	struct	oldnd_drop_site	*OlDnDDropSiteID;


/*********************/
/* OlDnDDragDropInfo */
/*********************/

typedef	struct _ol_dnd_root_info {
	Window		root_window;
	Position	root_x;
	Position	root_y;
	Time		drop_timestamp;
} OlDnDDragDropInfo, *OlDnDDragDropInfoPtr;

/*****************/
/* OlDnDSiteRect */
/*****************/

typedef XRectangle OlDnDSiteRect, *OlDnDSiteRectPtr;

/***********************/
/* OlDnDProtocolAction */
/***********************/

typedef	enum oldnd_protocol_action {
		OlDnDSelectionTransactionBegins,
		OlDnDSelectionTransactionEnds,
		OlDnDSelectionTransactionError,
		OlDnDDragNDropTransactionDone
}OlDnDProtocolAction;


/***********************************/
/* OlDnDProtocolActionCallbackProc */
/***********************************/

/* requestor callback resulting from calling OlDnD*SelectionTransaction */

#if	defined(__STDC__) || defined(__cplusplus)

typedef void		(*OlDnDProtocolActionCallbackProc)(Widget, Atom,
	OlDnDProtocolAction, Boolean, XtPointer);

#else	/* __STDC__ || __cplusplus */

typedef void		(*OlDnDProtocolActionCallbackProc)();

#endif	/* __STDC__ || 	__cplusplus */

typedef	OlDnDProtocolActionCallbackProc	OlDnDProtocolActionCbP;


/**********************************/
/* OlDnDSelectionTransactionState */
/**********************************/

typedef	enum oldnd_transaction_state {
		OlDnDTransactionBegins,
		OlDnDTransactionEnds,
		OlDnDTransactionDone,
		OlDnDTransactionRequestorError,
		OlDnDTransactionRequestorWindowDeath,
		OlDnDTransactionTimeout
} OlDnDTransactionState;

/*********************************/
/* OlDnDTransactionStateCallback */
/*********************************/

#if	defined(__STDC__) || defined(__cplusplus)

typedef void		(*OlDnDTransactionStateCallback)(Widget, Atom, 
	OlDnDTransactionState, Time, XtPointer);

#else	/* __STDC__ || __cplusplus */

typedef void		(*OlDnDTransactionStateCallback)();

#endif	/* __STDC__ || 	__cplusplus */


/*************************/
/* OLDnDSitePreviewHints */
/*************************/

typedef enum oldnd_site_preview_hints { 
		OlDnDSitePreviewNone,
		OlDnDSitePreviewEnterLeave = (1 << 0),
		OlDnDSitePreviewMotion = (1 << 1),
		OlDnDSitePreviewBoth = (OlDnDSitePreviewEnterLeave |
				 	OlDnDSitePreviewMotion),
		OlDnDSitePreviewDefaultSite = (1 << 2),
		OlDnDSitePreviewForwarded   = (1 << 3),
		OlDnDSitePreviewInsensitive = (1 << 4)
} OlDnDSitePreviewHints;

/*************************/
/* OlDnDTriggerOperation */
/*************************/

typedef enum oldnd_trigger_Operation {
		OlDnDTriggerCopyOp,
		OlDnDTriggerMoveOp
} OlDnDTriggerOperation;


/****************************************/
/* OlDnDRegister{Widget|Window}DropSite */
/****************************************/

#if	defined(__STDC__) || defined(__cplusplus)

typedef	void		(*OlDnDTriggerMessageNotifyProc)(
	Widget		widget,
	Window		window,
	Position	x,		/* in root window */
	Position	y,		/* in root window */
	Atom		selection,
	Time		timestamp,
	OlDnDDropSiteID	drop_site,
	OlDnDTriggerOperation operation,
	Boolean		send_done,
	Boolean		forwarded,	
	XtPointer	closure	
);

typedef	OlDnDTriggerMessageNotifyProc	OlDnDTMNotifyProc;

typedef void		(*OlDnDPreviewMessageNotifyProc)(
	Widget		widget,
	Window		window,
	Position	x,		/* in root window */
	Position	y,		/* in root window */
	int		type,		/* Enter, Leave or Motion */	
	Time		timestamp,
	OlDnDDropSiteID	drop_site,
	Boolean		forwarded,	
	XtPointer	closure	

);

typedef	OlDnDPreviewMessageNotifyProc	OlDnDPMNotifyProc;


/******************************/
/* OlDnDPreviewAndAnimate     */
/******************************/

typedef	void		(*OlDnDPreviewAnimateCallbackProc)(Widget, int, Time,
	Boolean, XtPointer);

typedef	OlDnDPreviewAnimateCallbackProc	OlDnDPreviewAnimateCbP;

#else	/* __STDC__ || __cplusplus */

typedef	void		(*OlDnDTriggerMessageNotifyProc)();

typedef	OlDnDTriggerMessageNotifyProc
			OlDnDTMNotifyProc;
	
typedef void		(*OlDnDPreviewMessageNotifyProc)();

typedef	OlDnDPreviewMessageNotifyProc
			OlDnDPMNotifyProc;

typedef	void		(*OlDnDPreviewAnimateCallbackProc)();

typedef	OlDnDPreviewAnimateCallbackProc
			OlDnDPreviewAnimateCbP;

#endif	/* __STDC__ || __cplusplus */


/**********************************/
/* OlDnD{Alloc|Free}TranisentAtom */
/**********************************/

#define FreeAllTransientAtoms	((Atom)-1)

/*
 * DragNDrop module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern Boolean		OlDnDDragAndDrop(Widget w, Window* window,
	Position* xPosition, Position* yPosition,
	OlDnDDragDropInfoPtr rootinfo, OlDnDPreviewAnimateCbP animate,
	XtPointer closure);

#else	/* __STDC__ || __cplusplus */

extern Boolean		OlDnDDragAndDrop();

#endif	/* __STDC__ || __cplusplus */


/*
 * OlDnDVCX module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern Atom		OlDnDAllocTransientAtom(Widget widget);

extern void		OlDnDBeginSelectionTransaction(Widget widget,
	Atom selection, Time timestamp, OlDnDProtocolActionCallbackProc proc, 
	XtPointer closure);

extern Boolean		OlDnDChangeDropSitePreviewHints(
	OlDnDDropSiteID dropsiteid, OlDnDSitePreviewHints hints);

extern void		OlDnDClearDragState(Widget widget);

extern Boolean		OlDnDDeliverPreviewMessage(Widget widget, Window root,
	Position rootx, Position rooty, Time timestamp);

extern Boolean		OlDnDDeliverTriggerMessage(Widget widget, Window root,
	Position rootx, Position rooty, Atom selection,
	OlDnDTriggerOperation operation, Time timestamp);

extern void		OlDnDDestroyDropSite(OlDnDDropSiteID dropsiteid);

extern void		OlDnDDisownSelection(Widget widget, Atom selection,
	Time time);

extern void		OlDnDDragNDropDone(Widget widget, Atom selection,
	Time timestamp, OlDnDProtocolActionCallbackProc proc,
	XtPointer closure);

extern void		OlDnDEndSelectionTransaction(Widget widget,
	Atom selection, Time timestamp, OlDnDProtocolActionCallbackProc proc,
	XtPointer closure);

extern void		OlDnDErrorDuringSelectionTransaction(Widget widget,
	Atom selection, Time timestamp, OlDnDProtocolActionCallbackProc proc,
	XtPointer closure);

extern void		OlDnDFreeTransientAtom(Widget widget, Atom atom);

extern Boolean		OlDnDGetCurrentSelectionsForWidget(Widget widget,
	Atom** atoms_return, Cardinal* num_sites_return);

extern OlDnDDropSiteID* OlDnDGetDropSitesOfWidget(Widget widget,
	Cardinal* num_sites_return);

extern OlDnDDropSiteID* OlDnDGetDropSitesOfWindow(Display* dpy, Window window, 
	Cardinal* num_sites_return);

extern Widget		OlDnDGetWidgetOfDropSite(OlDnDDropSiteID dropsiteid);
extern Window		OlDnDGetWindowOfDropSite(OlDnDDropSiteID dropsiteid);
extern void		OlDnDInitialize(Display* dpy);
extern Boolean		OlDnDInitializeDragState(Widget widget);

extern Boolean		OlDnDOwnSelection(Widget widget, Atom selection,
	Time timestamp, XtConvertSelectionProc convert_proc,
	XtLoseSelectionProc lose_selection_proc, XtSelectionDoneProc done_proc,
	OlDnDTransactionStateCallback state_proc, XtPointer closure);

extern Boolean		OlDnDOwnSelectionIncremental(Widget widget,
	Atom selection, Time timestamp,
	XtConvertSelectionIncrProc convert_incr_proc,
	XtLoseSelectionIncrProc lose_incr_selection_proc,
	XtSelectionDoneIncrProc incr_done_proc,
	XtCancelConvertSelectionProc incr_cancel_proc,
	XtPointer client_data, OlDnDTransactionStateCallback state_proc);

extern Boolean		OlDnDPreviewAndAnimate(Widget widget, Window root,
	Position rootx, Position rooty, Time timestamp,
	OlDnDPreviewAnimateCbP animate_proc, XtPointer closure);

extern Boolean		OlDnDQueryDropSiteInfo(OlDnDDropSiteID dropsiteid,
	Widget* widget, Window* window, OlDnDSitePreviewHints* preview_hints, 
	OlDnDSiteRectPtr* site_rects, unsigned int* num_rects,
	Boolean* on_interest);

extern OlDnDDropSiteID	OlDnDRegisterWidgetDropSite(Widget widget,
	OlDnDSitePreviewHints preview_hints, OlDnDSiteRectPtr site_rects,
	unsigned int num_sites, OlDnDTMNotifyProc tmnotify,
	OlDnDPMNotifyProc pmnotify, Boolean on_interest, XtPointer closure);

extern OlDnDDropSiteID	OlDnDRegisterWindowDropSite(Display* dpy, Window window,
	OlDnDSitePreviewHints preview_hints, OlDnDSiteRectPtr site_rects,
	unsigned int num_sites, OlDnDTMNotifyProc tmnotify,
	OlDnDPMNotifyProc pmnotify, Boolean on_interest, XtPointer closure);

extern Boolean		OlDnDSetDropSiteInterest(OlDnDDropSiteID dropsiteid,
	Boolean on_interest);

extern void		OlDnDSetInterestInWidgetHier(Widget widget,
	Boolean on_interest);

extern Boolean		OlDnDUpdateDropSiteGeometry(OlDnDDropSiteID dropsiteid,
	OlDnDSiteRectPtr site_rects, unsigned int num_rects);

extern void		OlDnDWidgetConfiguredInHier(Widget widget);

#else	/* __STDC__ || __cplusplus */

extern Atom		OlDnDAllocTransientAtom();
extern void		OlDnDBeginSelectionTransaction();
extern Boolean		OlDnDChangeDropSitePreviewHints();
extern void		OlDnDClearDragState();
extern Boolean		OlDnDDeliverPreviewMessage();
extern Boolean		OlDnDDeliverTriggerMessage();
extern void		OlDnDDestroyDropSite();
extern void		OlDnDDisownSelection();
extern void		OlDnDDragNDropDone();
extern void		OlDnDEndSelectionTransaction();
extern void		OlDnDErrorDuringSelectionTransaction();
extern void		OlDnDFreeTransientAtom();
extern Boolean		OlDnDGetCurrentSelectionsForWidget();
extern OlDnDDropSiteID* OlDnDGetDropSitesOfWidget();
extern OlDnDDropSiteID* OlDnDGetDropSitesOfWindow();
extern Widget		OlDnDGetWidgetOfDropSite();
extern Window		OlDnDGetWindowOfDropSite();
extern void		OlDnDInitialize();
extern Boolean		OlDnDInitializeDragState();
extern Boolean		OlDnDOwnSelection();
extern Boolean		OlDnDOwnSelectionIncremental();
extern Boolean		OlDnDPreviewAndAnimate();
extern Boolean		OlDnDQueryDropSiteInfo();
extern OlDnDDropSiteID	OlDnDRegisterWidgetDropSite();
extern OlDnDDropSiteID	OlDnDRegisterWindowDropSite();
extern Boolean		OlDnDSetDropSiteInterest();
extern void		OlDnDSetInterestInWidgetHier();
extern Boolean		OlDnDUpdateDropSiteGeometry();
extern void		OlDnDWidgetConfiguredInHier();

#endif	/* __STDC__ || __cplusplus */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_OLDNDVCX_H */
