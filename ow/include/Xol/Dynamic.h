#ifndef	_XOL_DYNAMIC_H
#define	_XOL_DYNAMIC_H

#pragma	ident	"@(#)Dynamic.h	302.2	92/04/16 include/Xol SMI"	/* olmisc:Dynamic.h 1.12	*/

/*
 *	Copyright (C) 1986,1992  Sun Microsystems, Inc
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
#include <Xol/buffutil.h>


#ifdef	__cplusplus
extern "C" {
#endif


typedef enum { 
	NOT_DETERMINED, MOUSE_CLICK, MOUSE_MOVE, MOUSE_MULTI_CLICK,
	MOUSE_MULTI_CLICK_PENDING, MOUSE_MULTI_CLICK_DONE
} ButtonAction;

typedef int		OlInputEvent;

typedef Bufferof(wchar_t)  WBuffer;

typedef struct {
	Boolean		consumed;
	XEvent*		event;
	KeySym*		keysym;
	char*		buffer;
	int*		length;
	OlInputEvent	ol_event;
} OlInputCallData, *OlInputCallDataPointer;


/*
 * function prototype section
 */

/*
 * Dynamic module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern void		DynamicHandler(Widget w);

extern OlInputEvent	LookupOlInputEvent(Widget w, XEvent* event,
	KeySym* keysym, char* * buffer, int* length);

extern ButtonAction	OlDetermineMouseAction(Widget w, XEvent* event);

#else	/* __STDC__ || __cplusplus */

extern void		DynamicHandler();
extern OlInputEvent	LookupOlInputEvent();
extern ButtonAction	OlDetermineMouseAction();

#endif	/* __STDC__ || __cplusplus */


/*
 * Olcommon module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern void		OlReplayBtnEvent(Widget w, XtPointer client_data,
	XEvent* event);

#else	/* __STDC__ || __cplusplus */

extern void		OlReplayBtnEvent();

#endif	/* __STDC__ || __cplusplus */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_DYNAMIC_H */
