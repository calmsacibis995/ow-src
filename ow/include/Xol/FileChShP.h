#ifndef	_XOL_FILECHSHP_H
#define	_XOL_FILECHSHP_H

#pragma	ident	"@(#)FileChShP.h	1.7	92/12/16 include/Xol SMI"	/* OLIT	*/

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
 *      Imported interfaces 
 *
 ************************************************************************/

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <X11/CoreP.h>
#include <X11/CompositeP.h>
#include <X11/ShellP.h>		/* Shell/WMShell/VendorShell/TransientShell */
#include <X11/VendorP.h>

#include <Xol/OpenLookP.h>

#include <Xol/FileChSh.h>


#ifdef	__cplusplus
extern "C" {
#endif


/* Define the instance part */
typedef struct _FileChooserShellPart {
	/* New resource fields */
	OlStrRep		text_format;
	Boolean         	pointer_warping;
	OlDefine		operation;
	Widget			file_chooser_widget;
	XtCallbackList		verify_callback;

	/* Internal fields */
	Widget			top_widget;
	OlStr			header;
	Boolean			positioned;
	Boolean     		pointer_warped;	/* unwarp only when no motion */
	int         		root_x;         /* to restore warped pointer */
	int         		root_y;         /* to restore warped pointer */
} FileChooserShellPart;

/* Define the full instance record */
typedef struct _FileChooserShellRec {
	CorePart		core;
	CompositePart		composite;
	ShellPart		shell;
	WMShellPart		wm_shell;
	VendorShellPart		vendor_shell;
	TransientShellPart	transient_shell;
	FileChooserShellPart	file_chooser_shell;
} FileChooserShellRec;


/* Define class part structure */
typedef struct _FileChooserShellClassPart {
	XtPointer		extension;
} FileChooserShellClassPart;

/* Define the full class record */
typedef struct _FileChooserShellClassRec {
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ShellClassPart		shell_class;
	WMShellClassPart	wm_shell_class;
	VendorShellClassPart	vendor_shell_class;
	TransientShellClassPart	transient_shell_class;
	FileChooserShellClassPart
				file_chooser_shell_class;
} FileChooserShellClassRec;

/* External definition for class record */
extern FileChooserShellClassRec fileChooserShellClassRec;


#ifdef	__STDC__
extern void	_OlFileChShPopDown(const Widget wid, 
	const Boolean override_pushpin);
#else	/* __STDC__ */
extern void	_OlFileChShPopDown();
#endif	/* __STDC__ */


#ifdef	__cplusplus
}
#endif


/* end of FileChShP.h */
#endif	/* _XOL_FILECHSHP_H */
