#ifndef	_XOL_FONTCHSHP_H
#define	_XOL_FONTCHSHP_H

#pragma       ident   "@(#)FontChShP.h	1.2    92/10/20 include/Xol SMI"    /* OLIT */

/*
 *	Copyright (C) 1986,1991  Sun Microsystems, Inc
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

#include	<Xol/FontChSh.h>

#include	<X11/ShellP.h>


#ifdef	__cplusplus
extern "C" {
#endif


typedef struct
{
	int keep_compiler_happy;   /* No new procedures */
} FontChooserShellClassPart;

typedef struct _FontChooserShellClassRec {
  	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ShellClassPart		shell_class;
	WMShellClassPart	wm_shell_class;
	VendorShellClassPart	vendor_shell_class;
	TransientShellClassPart		transient_shell_class;
	FontChooserShellClassPart	font_chooser_shell_class;
} FontChooserShellClassRec;

extern FontChooserShellClassRec fontChooserShellClassRec;

/* New fields for the OLIT FontChooserShell widget */

typedef struct {
	OlStrRep	text_format;
	Widget		fc_composite;
} FontChooserShellPart;

typedef struct _FontChooserShellRec
{
	CorePart 	core;
	CompositePart 	composite;
	ShellPart 	shell;
	WMShellPart	wm;
	VendorShellPart	vendor;
	TransientShellPart	transient;
	FontChooserShellPart	font_chooser;
} FontChooserShellRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_FONTCHSHP_H */
