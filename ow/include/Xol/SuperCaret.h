#ifndef	_SuperCaret_h
#define	_SuperCaret_h

#pragma ident "@(#)SuperCaret.h	1.3      92/10/22     include/Xol SMI"

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

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <Xol/OpenLook.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct _SuperCaretShellRec*	SuperCaretShellWidget;

extern	WidgetClass			superCaretShellWidgetClass;

#define	VisibilityIndeterminate	-1

extern	void	OlMoveWidget(const Widget	w,
			     const Position	x,
			     const Position	y
		);

extern	void	OlResizeWidget(const Widget	w,
			       const Dimension	width,
			       const Dimension	height,
			       const Dimension	border_width
		);

extern	void	OlConfigureWidget(const Widget		w,
				  const Position	x,
				  const Position	y,
				  const Dimension	width,
				  const Dimension	height,
				  const Dimension	border_width
		);

extern void	OlWidgetConfigured(const Widget	w);

extern void	OlSetSuperCaretFocusWidget(const Widget	w);
#ifdef	__cplusplus
}
#endif

#endif	/* _SuperCaret_h */
