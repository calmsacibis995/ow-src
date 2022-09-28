#ifndef	_XOL_ROOTSHELL_H
#define	_XOL_ROOTSHELL_H

#pragma	ident	"@(#)RootShell.h	302.6	94/01/14 include/Xol SMI"	/* OLIT	*/

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

#include <Xol/OpenLook.h>

#include <X11/Intrinsic.h>

#ifdef	__cplusplus
extern "C" {
#endif


/* Class record constants */

typedef struct _RootShellClassRec*	RootShellWidgetClass;
typedef struct _DisplayShellClassRec*	DisplayShellWidgetClass;
typedef struct _ScreenShellClassRec*	ScreenShellWidgetClass;


externalref WidgetClass		rootShellWidgetClass;
externalref WidgetClass		displayShellWidgetClass;
externalref WidgetClass		screenShellWidgetClass;


#if	defined(__STDC__) || defined(__cplusplus)

typedef void		(*OlDynamicScreenCallback)(Screen* screen,
	XtPointer closure);

#else	/* __STDC__ || __cplusplus */

typedef void		(*OlDynamicScreenCallback)();

#endif	/* __STDC__ || __cplusplus */


#if	defined(__STDC__) || defined(__cplusplus)

extern void		OlAddDynamicScreenCB(Screen* screen,
	OlDynamicScreenCallback proc, XtPointer closure);

extern void		OlCallDynamicScreenCBs(Screen* screen);

extern Widget		OlCreateDisplayShell(Widget w, ArgList args,
	Cardinal num_args);

extern Widget		OlCreateScreenShell(Widget w, ArgList args,
	Cardinal num_args);

extern void		OlDestroyDisplayShell(Widget widget, Boolean xpd);
extern void		OlDestroyScreenShell(Widget widget, Boolean xpd);
extern Atom		OlInternAtom(Display* dpy, const char *name);

extern void		OlRemoveDynamicScreenCB(Screen* screen,
	OlDynamicScreenCallback proc, XtPointer closure);

extern Widget		_OlGetDisplayShellOfScreen(Screen* scr);
extern Widget		_OlGetDisplayShellOfWidget(Widget w);

extern void		_OlGetListOfDisplayShells(Widget* * list,
	Cardinal* num);

extern Cardinal		_OlGetMultiClickTimeout(Widget w);
extern Widget		_OlGetScreenShellOfScreen(Screen* scr);
extern Widget		_OlGetScreenShellOfWidget(Widget w);
extern void		_OlInitAttributes(Widget w);
extern Boolean		_OlCtrlAltMetaKey(Display* dpy);
extern Boolean		_OlUseShortOLWinAttr(Display* dpy);
extern Boolean		_OlWidgetOnXtGrabList(Widget widget);

extern void		_OlCreateImVSInfo(Widget vw);
extern void		_OlDestroyImVSInfo(Widget vw);



#else	/* __STDC__ || __cplusplus */

extern void		OlAddDynamicScreenCB();
extern void		OlCallDynamicScreenCBs();
extern Widget		OlCreateDisplayShell();
extern Widget		OlCreateScreenShell();
extern void		OlDestroyDisplayShell();
extern void		OlDestroyScreenShell();
extern Atom		OlInternAtom();
extern void		OlRemoveDynamicScreenCB();

extern Widget		_OlGetDisplayShellOfScreen();
extern Widget		_OlGetDisplayShellOfWidget();
extern void		_OlGetListOfDisplayShells();
extern Cardinal		_OlGetMultiClickTimeout();
extern Widget		_OlGetScreenShellOfScreen();
extern Widget		_OlGetScreenShellOfWidget();
extern void		_OlInitAttributes();
extern Boolean		_OlUseShortOLWinAttr();
extern Boolean		_OlWidgetOnXtGrabList();

extern void		_OlCreateImVSInfo();
extern void		_OlDestroyImVSInfo();

#endif	/* __STDC__ || __cplusplus */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_ROOTSHELL_H */
