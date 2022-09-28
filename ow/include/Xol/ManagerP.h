#ifndef	_XOL_MANAGERP_H
#define	_XOL_MANAGERP_H

#pragma	ident	"@(#)ManagerP.h	302.2	93/01/20 include/Xol SMI"	/* manager:ManagerP.h 1.13	*/

/*
 *        Copyright (C) 1986,1991  Sun Microsystems, Inc
 *                    All rights reserved.
 *          Notice of copyright on this source code 
 *          product does not indicate publication. 
 * 
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by 
 * the U.S. Government is subject to restrictions as set forth 
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988) 
 * and FAR 52.227-19 (c) (June 1987).
 *
 *    Sun Microsystems, Inc., 2550 Garcia Avenue,
 *    Mountain View, California 94043.
 *
 */

/***********************************************************************
 *
 * Manager Widget Private Data
 *
 ***********************************************************************/


#include <Xol/Manager.h>
#include <Xol/OpenLookP.h>

#include <X11/IntrinsicP.h>
#include <X11/ConstrainP.h>


#ifdef	__cplusplus
extern "C" {
#endif

/***********************************************************************
 *
 * Private Callbacks
 *
 ***********************************************************************/


/*
 * callback structure for manager XtNpreSelect and
 * XtNpostSelect callbacks.
 *
 * these callbacks are invoked by widgets supporting
 * XtNSelect and XtNunselect callbacks, prior to
 * and immedaitely after, these callbacks have been
 * invoked.
 */

#define	OL_REASON_PRE_SELECT		8
#define	OL_REASON_POST_SELECT		9

typedef struct {
	int		reason;
	XEvent*		event;
	Widget		selectee;
} OlManagerSelectCallbackStruct;

/***********************************************************************
 *
 * Class record
 *
 ***********************************************************************/

/* New fields for the ManagerWidget class record */
typedef struct {
    OlHighlightProc	highlight_handler;
    XtPointer		reserved1;	/* obsolete insert_descendant */
    XtPointer		reserved2;	/* obsolete delete_descendant */
    OlTraversalFunc	traversal_handler;
    OlActivateFunc	activate;
    OlEventHandlerList	event_procs;
    Cardinal		num_event_procs;
    OlRegisterFocusFunc	register_focus;
    XtPointer		reserved4;
    XtVersionType	version;
    XtPointer		extension;
    _OlDynData		dyn_data;
    OlTransparentProc	transparent_proc;
    SuperCaretQueryLocnProc query_sc_locn_proc;
} ManagerClassPart;

/* Full class record declaration */
typedef struct _ManagerClassRec {
    CoreClassPart	core_class;
    CompositeClassPart	composite_class;
    ConstraintClassPart	constraint_class;
    ManagerClassPart	manager_class;
} ManagerClassRec;

extern ManagerClassRec	managerClassRec;

/***********************************************************************
 *
 * Instance record
 *
 ***********************************************************************/

/* New fields for the ManagerWidget record */
typedef struct _ManagerPart {
    /* Resource-related data */
    XtPointer		user_data;
    XtCallbackList	consume_event;
    Pixel		input_focus_color;
    String		reference_name;
    Widget		reference_widget;
    Boolean		traversal_on;

    /* Non-resource-related data */
    Boolean		has_focus;
    unsigned char	dyn_flags;

    XtCallbackList	unrealize_callbacks;

    XtCallbackList	pre_select;
    XtCallbackList	post_select;
} ManagerPart;


/* Full instance record declaration */
typedef struct _ManagerRec {
    CorePart		core;
    CompositePart	composite;
    ConstraintPart	constraint;
    ManagerPart		manager;
} ManagerRec;


/***********************************************************************
 *
 * Constants
 *
 ***********************************************************************/

#define MGRPART(w)		( &(((ManagerWidget)(w))->manager) )
#define MGRCLASSPART(wc)	( &(((ManagerWidgetClass)(wc))->manager_class) )
#define _OlIsManager(w)		XtIsSubclass((w), managerWidgetClass)
#define _OL_IS_MANAGER		_OlIsManager

/* dynamic resources bit values */
#define OL_B_MANAGER_BG			(1 << 0)
#define OL_B_MANAGER_FOCUSCOLOR		(1 << 1)
#define OL_B_MANAGER_BORDERCOLOR	(1 << 2)


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_MANAGERP_H */
