#ifndef	_XOL_FILECHSH_H
#define	_XOL_FILECHSH_H

#pragma	ident	"@(#)FileChSh.h	1.8	93/02/25 include/Xol SMI"	/* OLIT	*/

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
 *      Interface of the OLIT file chooser shell widget
 *
 ************************************************************************/

/************************************************************************
 *
 *      Imported interfaces 
 *
 ************************************************************************/

#include <Xol/FileCh.h>
#include <Xol/OpenLook.h>	/* OL_* */


#ifdef	__cplusplus
extern "C" {
#endif


typedef struct _FileChooserShellClassRec*	FileChooserShellWidgetClass;
typedef struct _FileChooserShellRec*		FileChooserShellWidget;

extern WidgetClass fileChooserShellWidgetClass;


/* Callback structure definitions */
typedef struct _OlFileChShVerifyCallbackStruct {

	/* OLIT standard fields */
	int			reason;	/* OL_REASON_VERIFY */

	/* FileChooser standard fields */
	XtPointer		extension;	/* reserved */
	OlDefine		operation;	/* OL_OPEN, ... */
	
	/* Callback-dependent fields */
	Boolean			accept_verify;	/* set to FALSE to reject */

} OlFileChShVerifyCallbackStruct;


#ifdef	__cplusplus
}
#endif


/* end of FileChSh.h */
#endif	/* _XOL_FILECHSH_H */
