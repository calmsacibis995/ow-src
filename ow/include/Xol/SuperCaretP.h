#ifndef	_SuperCaretP_h
#define	_SuperCaretP_h

#pragma ident "@(#)SuperCaretP.h	1.4      93/02/08     include/Xol SMI"

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

#include <X11/IntrinsicP.h>
#include <X11/CoreP.h>
#include <X11/CompositeP.h>
#include <X11/ShellP.h>

#include <Xol/SuperCaret.h>
#include <Xol/RootShellP.h>

#ifdef	__cplusplus
extern "C" {
#endif

/************************************************************************
 *
 * the class part and class record
 *
 ************************************************************************/

typedef	void	(*SuperCaretShapeProc)(SuperCaretShellWidget	newscsw,
				       SuperCaretShellWidget	oldcsw
				      );

#define	XtInheritSuperCaretShapeProc	((SuperCaretShapeProc)_XtInherit)

typedef	void	(*SuperCaretUpdateProc)(SuperCaretShellWidget	newscsw,
					SuperCaretShellWidget	oldscsw
				       );

#define	XtInheritSuperCaretUpdateProc	((SuperCaretUpdateProc)_XtInherit)

typedef	void	(*SuperCaretConfigureProc)(SuperCaretShellWidget scsw,
					   unsigned int		 request_mode,
					   const Boolean	 overrirde
					  );

#define	XtInheritSuperCaretConfigureProc ((SuperCaretConfigureProc)_XtInherit)

typedef	void	(*SuperCaretPopupProc)(SuperCaretShellWidget	scsw,
				       const Boolean		override
				      );

#define	XtInheritSuperCaretPopupProc ((SuperCaretPopupProc)_XtInherit)


#define XtInheritSuperCaretSiblingExposeProc ((SuperCaretSiblingExposeProc)_XtInherit)

typedef	struct _SuperCaretShellClassPart {
	SuperCaretShapeProc		shape_supercaret;
	SuperCaretUpdateProc		update_supercaret;
	SuperCaretConfigureProc		configure_supercaret;
	SuperCaretPopupProc		popup_supercaret;
	XtEventHandler			handle_supercaret_events;
	EventMask			supercaret_event_mask;
	XtEventHandler			handle_focus_widget_events;
	EventMask			focus_widget_event_mask;
	
	XtPointer			extension;
} SuperCaretShellClassPart;

typedef	struct _SuperCaretShellClassRec {
	CoreClassPart			core_class;
	CompositeClassPart		composite_class;
	ShellClassPart			shell_class;
	OverrideShellClassPart		override_shell_class;
	SuperCaretShellClassPart	supercaret_shell_class;
} SuperCaretShellClassRec, *SuperCaretShellWidgetClass;

/************************************************************************
 *
 * the instance part and instance record
 *
 ************************************************************************/

typedef	struct _SuperCaretShellPart {
	Pixel				foreground;
	GC				gc;
	Boolean				is_shaped;

	Widget				current_parent;

	Boolean				in_setvalues;
	Boolean				in_update_supercaret;

	unsigned int			pending_configures;
	Boolean				mapped;

	Widget				focus_widget;
	int				focus_visibility;
	unsigned int			map_state;

	unsigned int			scale;
	unsigned int 			actual_sc_scale;

	int				visibility;
	int				anticipated;

	SuperCaretShape			supercaret_shape;

	XtCallbackList			unrealize_callbacks;
	Pixmap				sc_left, sc_left_mask;
	Pixmap				sc_bottom, sc_bottom_mask;
	Pixmap				left, bottom;
	Pixmap				bitmap_left, bitmap_bottom;

	XtPointer			extension;
} SuperCaretShellPart;

typedef struct _SuperCaretShellRec {
	CorePart		core;
	CompositePart		composite;
	ShellPart		shell;
	OverrideShellPart	override;
	SuperCaretShellPart	supercaret;
}SuperCaretShellRec;

#ifdef	__cplusplus
}
#endif
#endif	/* _SuperCaretP_h */
