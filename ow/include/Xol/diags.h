#ifndef	_XOL_DIAGS_H
#define	_XOL_DIAGS_H

#pragma	ident	"@(#)diags.h	1.9	93/05/04 include/Xol SMI"	/* OLIT	493 */

/*
 *	Copyright (C) 1992  Sun Microsystems, Inc
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


/************************************************************************
 *
 *      Interface of the diagnostics module
 *
 *	Provides general purpose utilities.
 *
 ************************************************************************/

/************************************************************************
 *
 *      Imported interfaces 
 *
 ************************************************************************/

#include <stdio.h>		/* NULL */
#include <stdlib.h>		/* exit(), malloc(), calloc(), free() */

#include <X11/Intrinsic.h>	/* String, XtMalloc() */
#include <X11/IntrinsicP.h>	/* XtWidgetProc */

#include <Xol/OpenLook.h>	/* OlStr */


#ifdef	__cplusplus
extern "C" {
#endif


/************************************************************************
 *
 *      Module Public Macro Definitions
 *
 ************************************************************************/

#ifndef	DEBUG
#define	NDEBUG
#endif	/* DEBUG */
 
#define	BAD_SYSCALL	(-1)


/************************************************************************
 *
 *      _OL_CALLOC -- Allocate dynamic typed storage array
 *		with error checking
 *
 ************************************************************************/

#define	_OL_CALLOC(ptr, nelem, type) \
	(ptr) = (type*)XtCalloc((Cardinal)(nelem), (Cardinal)sizeof (type))


/************************************************************************
 *
 *      _OL_MALLOC -- Allocate dynamic typed storage with error checking
 *
 ************************************************************************/

#define	_OL_MALLOC(ptr, type) \
	(ptr) = (type*)XtMalloc((Cardinal)sizeof (type))


/************************************************************************
 *
 *      _OL_MALLOCN -- Allocate dynamic generic storage with error checking
 *
 ************************************************************************/

#define	_OL_MALLOCN(ptr, nelem) \
	(ptr) = (void*)XtMalloc((Cardinal)(nelem))


/************************************************************************
 *
 *      EFREE -- Free dynamic storage with error checking
 *
 ************************************************************************/

#ifdef	DEBUG
	#define	_OL_FREE(ptr) \
		{ \
			if (NULL == ptr) { \
				_OlMessage("Attempt to free null address.\n"); \
				_OL_WHERE(); \
			} \
			\
			XtFree((char*)ptr); \
			(ptr) = NULL; \
		}
#else
	#define	_OL_FREE(ptr) \
		{ \
			XtFree((char*)ptr); \
			(ptr) = NULL; \
		}
#endif	/* DEBUG */


/************************************************************************
 *
 *      _OL_WHERE -- Print the calls location in the source file
 *
 ************************************************************************/

#define	_OL_WHERE() \
	_OlMessage("At line %d in file %s.\n", __LINE__, __FILE__)		


/************************************************************************
 *
 *      Module Exported Function Declarations
 *
 ************************************************************************/

extern void		_OlMessage(const char *const fmt, ...);

extern void		_OlReport(const char *const func_name,
	const char *const fmt, ...);

extern void		_OlAbort(const char *const func_name, 
	const char*const fmt, ...);

extern Boolean		_OlInArgList(const char*const resource_name, 
	const ArgList args, const Cardinal num_args);

extern Boolean		_OlStrIsEmpty(const OlStr olstr, 
	const OlStrRep text_format);

extern const Widget	_OlRootWidget(const Widget widget);

#ifdef	DEBUG

extern void		_OlWidgetPrintInstanceTree(const Widget widget);
extern const String	_OlWidgetClassOfAncestory(const Widget widget);
extern const String	_OlWidgetInstanceHierarchy(const Widget widget);

extern String		_OlWidgetResourceValue(const Widget widget, 
	const char*const resource_name);

extern const Widget	_OlWidgetOfName(const Widget widget, 
	const char*const widget_name);

extern void		_OlMain(const int argc, const char*const argv[], 
	const char** fallback_resources, XtWidgetProc construct_basewin);

extern void		_OlDiagsTest(const int argc, const char*const argv[]);

#endif	/* DEBUG */


#ifdef	__cplusplus
}
#endif


/* end of diags.h */
#endif	/* _XOL_DIAGS_H */
