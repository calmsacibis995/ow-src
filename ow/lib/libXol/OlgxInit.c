#pragma ident	"@(#)OlgxInit.c	1.7	97/03/26 lib/libXol SMI"	/* olg:OlgInit.c 302.7	*/

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

/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* Initialize device structures */


#include <libintl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Intrinsic.h>

#include <Xol/OlgxP.h>
#include <Xol/OpenLook.h>
#include <Xol/RootShellP.h>


static _OlgxDevice	*_OlgxDeviceHead;

/* Inactive pattern */

#define inactive_width 8
#define inactive_height 2

static unsigned char inactive_bits[] = {
   0x55, 0xaa,
};


/* Get Device Data
 *
 * Create a Device structure (if one for the requested resolution and scale
 * does not already exist) and initialize it.  The arc descriptions are
 * initialized immediately; bitmap initialization is deferred until needed.
 * Use compiled in versions where reasonable, otherwise, try to read the
 * arc descriptions from a file.  If the read fails, use the compiled-in
 * defaults.
 */

_OlgxDevice *
_OlgxGetDeviceData (Widget w, int scale)
{
    register Screen		*scr = XtScreenOfObject(w);
    register _OlgxDevice	*pDev;	/* ptr to device structure */
    Colormap	cmap;			/* Object's colormap */
    Pixmap	pixmap;			/* Used in GC creation */
    char	tmp;			/* garbage value */

    cmap = OlColormapOfObject(w);

    /* Check if the structure we need has already been allocated. */
    /* The pDev structures are now cached on screen, scale and colormap */
    /* This implicitly caches on resolution and depth too - */
    /*    same screen implies same resolution */
    /*    same colormap (per screen) implies same depth */
    for (pDev=_OlgxDeviceHead; pDev; pDev=pDev->next)
    {
	if (pDev->scr == scr && pDev->scale == scale && pDev->cmap == cmap)
	{
	    pDev->refCnt++;
	    return pDev;
	}
    }

    /* Allocate new structure and put it at the head of the list. */
    pDev = (_OlgxDevice *) XtCalloc (1, sizeof (_OlgxDevice));
    pDev->next = _OlgxDeviceHead;
    _OlgxDeviceHead = pDev;
    pDev->refCnt = 1;

    /* Initialize all non-pixmap fields (well, OK, initialize
     * the stipples, too)
     */
    pDev->scr = scr;
    pDev->scale = scale;
    pDev->cmap = cmap;
    pDev->depth = OlDepthOfObject(w);
    pDev->threed = OlgIs3d(XtScreenOfObject(w));


    /* OLGX_TODO: Do we want to remove the inactiveStipple and the
     */
    pDev->inactiveStipple = XCreateBitmapFromData (DisplayOfScreen (scr),
	       RootWindowOfScreen (scr), (char *) inactive_bits,
	       inactive_width, inactive_height);

    if (!pDev->inactiveStipple)
    {
	OlError (dgettext(OlMsgsDomain,
		"_OlgxGetDeviceData:  Can't create stipple bitmaps"));
    }

    return pDev;

} /* _OlgxGetDeviceData */

static void
_FreeDeviceData	(register _OlgxDevice **pDev, Boolean must_destroy)
{
	register _OlgxDevice	*dev = *pDev;

	 if (--(dev->refCnt) == 0 || must_destroy) {

	/* OLGX_TODO - remove following code when no longer needed */

		XFreePixmap(DisplayOfScreen(dev->scr), dev->inactiveStipple);

		if (dev->dropTarget != (Pixmap)NULL)
			XFreePixmap(DisplayOfScreen(dev->scr), dev->dropTarget);

	/* OLGX_TODO: end of code to be removed */

		*pDev = dev->next;	/* unchain */

		XtFree((char *)dev);
	}
} /* _FreeDeviceData */

void
_OlgxFreeDeviceData (Screen *screen)
{
	register _OlgxDevice	**pDev;	/* ptr to device structure */

	for (pDev = &_OlgxDeviceHead; *pDev;) {
		if ((*pDev)->scr == screen)
			_FreeDeviceData(pDev, True);
		else
			 pDev = &((*pDev)->next);
	}
} /* _OlgxFreeDeviceData */

void
_OlgxFreeDeviceDataRef ( dev )
_OlgxDevice *		dev;
{
	register _OlgxDevice	**pDev;	/* ptr to device structure */
	for (pDev = &_OlgxDeviceHead; *pDev;) {
		if (*pDev == dev) {
			_FreeDeviceData(pDev, False);
			break;
		} else
			 pDev = &((*pDev)->next);
	}
} /* _OlgxFreeDeviceDataRef */
