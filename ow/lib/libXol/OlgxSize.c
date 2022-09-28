#pragma ident	"@(#)OlgxSize.c	1.6	97/03/26 lib/libXol SMI"	/* olg:OlgSize	*/

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


#include <Xol/OlgxP.h>
#include <libintl.h>

/* Size an oblong button.
 *
 * Determine the optimum size for an oblong button.  The button size is
 * calculated to "just fit" around the label.  The size of the label is
 * determined by the function sizeProc.  Flags describing the button
 * attributes are the same as for olgx_draw_button.
 */
void
OlgxSizeOblongButton(Screen*  scr, OlgxAttrs*  pInfo,
		     XtPointer label, OlSizeProc sizeProc,
		     unsigned flags,
		     Dimension* pWidth, Dimension* pHeight)
{
    Dimension	lblHeight;

    /* get the actual label size to find how much the corners must be
     * separated.
     */
    (*sizeProc) (scr, pInfo, label, pWidth, &lblHeight);

    if ((flags & OLGX_LABEL_IS_XIMAGE) || (flags & OLGX_LABEL_IS_PIXMAP))
	*pHeight = lblHeight + (Dimension)ButtonSpace_Width(pInfo->ginfo);
    else {
	/* button size is determined by size of glyph (unless this is too
	 * small to accommodate text height)
	 */
	*pHeight = (Dimension)Button_Height(pInfo->ginfo);
	if (*pHeight < lblHeight)
	    *pHeight = lblHeight + (Dimension)ButtonSpace_Width(pInfo->ginfo);
    }

    /* Add in the size of the menu mark */
    if (flags & OLGX_MENU_MARK)
	*pWidth += (Dimension)(ButtonSpace_Width(pInfo->ginfo) +
	    MenuMark_Width(pInfo->ginfo));

    /* Add in the size of the corners */
    *pWidth += (Dimension)(2*ButtonEndcap_Width(pInfo->ginfo));
}

/* Size a rectangular button.
 *
 * Determine the optimum size for a rectangular button.  The button size is
 * calculated to "just fit" around the label.  The size of the label is
 * determined by the function sizeProc.  Flags describing the button
 * attributes are the same as for olgx_draw_choice_item.
 */

void
OlgxSizeRectButton (Screen *scr, OlgxAttrs *pInfo, XtPointer label,
                   OlSizeProc sizeProc, unsigned int flags,
                   Dimension *pWidth, Dimension *pHeight)
{
    int		minHeight;

    /* get the actual label size. */
    (*sizeProc) (scr, pInfo, label, pWidth, pHeight);

    /* Add in the size of the borders */
    *pWidth += (Dimension)(2 * ButtonSpace_Width(pInfo->ginfo));
    *pHeight += (Dimension)ButtonSpace_Width(pInfo->ginfo);
}
 
