#ifndef	_XOL_FONTCHSH_H
#define	_XOL_FONTCHSH_H

#pragma       ident   "@(#)FontChSh.h	1.4    92/12/04 include/Xol SMI"    /* OLIT */

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

#include <X11/Shell.h>
#include <Xol/FontCh.h>

#ifdef	__cplusplus
extern "C" {
#endif


typedef struct _FontChooserShellClassRec	*FontChooserShellWidgetClass;
typedef struct _FontChooserShellRec		*FontChooserShellWidget;

extern WidgetClass fontChooserShellWidgetClass;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_FONTCHSH_H */
