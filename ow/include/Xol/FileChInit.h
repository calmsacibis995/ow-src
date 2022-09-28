#ifndef	_XOL_FILECHINIT_H
#define	_XOL_FILECHINIT_H

#pragma ident	"@(#)FileChInit.h	1.4    92/12/16 include/Xol SMI"    /* OLIT	493	*/

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
 *      Initialization-specific interfaces of the file chooser panel widget
 *
 ************************************************************************/


/************************************************************************
 *
 *      Imported interfaces 
 *
 ************************************************************************/

#include <X11/Intrinsic.h>	/* Widget, ArgList, Cardinal */


#ifdef	__cplusplus
extern "C" {
#endif


/************************************************************************
 *
 *	External Private Interface Declarations
 *
 ************************************************************************/

						/* class procedures */

extern void	_OlFileChInitialize(Widget request, Widget c_new, ArgList args,
	Cardinal* num_args);

						/* private procedures */

extern void	_OlVerifyOperationValue(OlDefine* operationp);
extern Boolean	_OlFileChVerifyFolderValue(String* folderp);

extern void	_OlFileChUpdateCurrentFolderDisplay(const Widget wid, 
	const char*const string);

extern void	_OlFileChSetListPrompt(const Widget c_new);
extern void	_OlFileChSetDocumentLabel(const Widget c_new);
extern void	_OlFileChSetDocumentString(const Widget c_new);


#ifdef	__cplusplus
}
#endif


/* end of %M */
#endif	/* _XOL_FILECHINIT_H */
