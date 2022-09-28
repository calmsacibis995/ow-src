#ifndef	_XOL_FILECHIMPL_H
#define	_XOL_FILECHIMPL_H

#pragma ident	"@(#)FileChImpl.h	1.6    93/02/04 include/Xol SMI"    /* OLIT	493	*/

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
 *      Implementation-specific interfaces of the file chooser panel widget
 *
 ************************************************************************/


/************************************************************************
 *
 *      Imported interfaces 
 *
 ************************************************************************/

#include <X11/Intrinsic.h>	/* Widget, XtPointer, String, Boolean */

#include <Xol/Datum.h>
#include <Xol/FileChP.h>


#ifdef	__cplusplus
extern "C" {
#endif


/************************************************************************
 *
 *	External Private Interface Declarations
 *
 ************************************************************************/

						/* callback procedures */

extern void	_OlFileChGotoCB(Widget widget, XtPointer client_data,
	XtPointer call_data);

extern void	_OlFileChGotoTypeInCB(Widget widget, XtPointer client_data,
	XtPointer call_data);

extern void	_OlFileChGotoFolderCB(Widget widget, XtPointer client_data,
	XtPointer call_data);

extern void	_OlFileChDocumentTypeInCB(Widget widget, XtPointer client_data, 
	XtPointer call_data);

extern void	_OlFileChOpenCB(Widget widget, XtPointer client_data,
	XtPointer call_data);

extern void	_OlFileChCommandCB(Widget widget, XtPointer client_data,
	XtPointer call_data);

extern void	_OlFileChItemCurrentCB(Widget widget, XtPointer client_data, 
	XtPointer call_data);

extern void	_OlFileChListEventHandler(Widget widget, XtPointer client_data, 
	XEvent* event, Boolean* continue_to_dispatch);

extern void	_OlFileChCancelCB(Widget widget, XtPointer client_data,
	XtPointer call_data);

extern void	_OlFileChUpdateHistory(FileChooserPart* my);


						/* private procedures */

extern void	_OlFileChCallbackOpenFolder(const FileChooserWidget fcw, 
	String folder);

extern Boolean	_OlFileChFillList(const Widget wid);


#ifdef	DEBUG
extern void	_OlFileChTest(const int argc, 
	const char* const argv[]);
#endif	/* DEBUG */


#ifdef	__cplusplus
}
#endif


/* end of FileChImpl.h */
#endif	/* _XOL_FILECHIMPL_H */
