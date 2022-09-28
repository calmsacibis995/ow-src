#pragma ident	"@(#)OlgxButton.c	1.17	97/03/26 lib/libXol SMI"     /*"@(#)olg:OlgLabel.c     302.7"*/

/*
 *      Copyright (C) 1986,1992  Sun Microsystems, Inc
 *                      All rights reserved.
 *              Notice of copyright on this source code
 *              product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *      Sun Microsystems, Inc., 2550 Garcia Avenue,
 *      Mountain View, California 94043.
 *
 */

/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <widec.h>

#include <X11/Intrinsic.h>

#include <Xol/OlgxP.h>
#include <Xol/OpenLook.h>
#include <Xol/RootShell.h>


static unsigned char	mnemonicString[] = "(x)";


/* Determine the width and height in pixels of each of the component parts
 * of a label.  All parts are assumed to have the same height.
 */

static void
sizeLabelParts (Screen *scr, OlgxTextLbl *labeldata, Dimension *labelWidth, Dimension *mnemonicWidth, Dimension *popupWidth, Dimension *qWidth, Dimension *accWidth, Dimension *height)
{
    int		dir, ascent, descent;
    XCharStruct	overall;
    OlDefine	displayStyle;

    XTextExtents((XFontStruct *)labeldata->font, (char *)labeldata->label, 
	strlen ((char *)labeldata->label), &dir, &ascent, &descent, &overall);
    *labelWidth = overall.width;
    *height = ascent + descent;

    *mnemonicWidth = 0;
    if (labeldata->mnemonic) {

	displayStyle = OlQueryMnemonicDisplay(_OlGetScreenShellOfScreen(scr));
	if (displayStyle == OL_DISPLAY)
	    displayStyle = OL_UNDERLINE;
	if (displayStyle == OL_HIGHLIGHT || displayStyle == OL_UNDERLINE) {
	    if (!strchr((const char *)labeldata->label, 
			(int)labeldata->mnemonic)) {
		char	mnemonic;
		Boolean	found = False;

		if (isalpha (labeldata->mnemonic)) {
		    mnemonic = islower (labeldata->mnemonic) ?
			toupper (labeldata->mnemonic) :
			tolower (labeldata->mnemonic);

		    if (strchr((const char *)labeldata->label, (int)mnemonic)) {
			found = True;
			labeldata->mnemonic = mnemonic;
		    }
		}

		if (!found) {
		    mnemonicString [1] = labeldata->mnemonic;
		    *mnemonicWidth = XTextWidth((XFontStruct *)labeldata->font,
			(char *)mnemonicString, 3);
		}
	    }
	}
    }

    if (labeldata->flags & TL_POPUP)
	*popupWidth = XTextWidth ((XFontStruct *)labeldata->font, "...", 3);
    else
	*popupWidth = 0;

    displayStyle = OlQueryAcceleratorDisplay(_OlGetScreenShellOfScreen(scr));
    if (labeldata->accelerator && displayStyle == OL_DISPLAY) {

	if (labeldata->qualifier)
	    *qWidth = XTextWidth((XFontStruct *)labeldata->font,
		(char *)labeldata->qualifier,
		strlen((char *)labeldata->qualifier));
	else
	    *qWidth = 0;

	if (labeldata->qualifier || labeldata->meta)
	    /* always use widest char, so accelerator fields will line up */
	    *accWidth = XTextWidth((XFontStruct *)labeldata->font, "M", 1);
	else
	    /* user specified XtNacceleratorText directly, so get its width */
	    *accWidth = XTextWidth((XFontStruct *)labeldata->font,
		(char *)labeldata->accelerator,
		strlen((char *)labeldata->accelerator));
    } else {
	*qWidth = 0;
	if (displayStyle == OL_DISPLAY && 
		(labeldata->flags & TL_LAYOUT_WITH_META))
	    *accWidth = XTextWidth((XFontStruct *)labeldata->font, "M", 1);
	else
	    *accWidth = 0;
    }

}

static void
MbsizeLabelParts (Screen *scr, OlgxTextLbl *labeldata, Dimension *labelWidth, Dimension *mnemonicWidth, Dimension *popupWidth, Dimension *qWidth, Dimension *accWidth, Dimension *height)
{
    int		dir, ascent, descent;
    XCharStruct	overall;
    XRectangle	overall_ink_extents;
    XRectangle 	overall_logical_extents;
    OlDefine	displayStyle;

    if (labeldata->text_format == OL_WC_STR_REP) {
	XwcTextExtents((XFontSet)labeldata->font, 
			    (wchar_t *)labeldata->label, 
			    wslen ((wchar_t *)labeldata->label), 
			    &overall_ink_extents, 
			    &overall_logical_extents);
    } else {
	XmbTextExtents((XFontSet)labeldata->font, 
		    (char *)labeldata->label, 
		    strlen ((char *)labeldata->label), 
		    &overall_ink_extents, 
		    &overall_logical_extents);
    }

    *labelWidth = overall_logical_extents.width;
    *height = overall_logical_extents.height;

    *mnemonicWidth = 0;
    if (labeldata->mnemonic) {

	displayStyle = OlQueryMnemonicDisplay(_OlGetScreenShellOfScreen(scr));
	if (displayStyle == OL_DISPLAY)
	    displayStyle = OL_UNDERLINE;
	if (displayStyle == OL_HIGHLIGHT || displayStyle == OL_UNDERLINE) {
	    wchar_t wmnemonic;

            (void)mbtowc(&wmnemonic, (const char *)&labeldata->mnemonic,1);

	    if (( (labeldata->text_format == OL_WC_STR_REP) ?
		(wschr((wchar_t *)labeldata->label, (int)wmnemonic)
				== (wchar_t *)NULL) :
		(strchr ((const char *)labeldata->label, 
					(int)labeldata->mnemonic) 
					== (char *)NULL) )) {
		unsigned char	mnemonic;
		Boolean	found = False;

		if (isalpha (labeldata->mnemonic)) {
		    mnemonic = islower (labeldata->mnemonic) ?
			toupper (labeldata->mnemonic) :
			tolower (labeldata->mnemonic);

	    	    (void)mbtowc(&wmnemonic, (const char *)&mnemonic,1);
	    	    if (( (labeldata->text_format == OL_WC_STR_REP) ?
			(wschr((wchar_t *)labeldata->label, (int)wmnemonic)
					!= (wchar_t *) NULL) :
			(strchr((const char *)labeldata->label, (int)mnemonic)
					!= (char *)NULL ) )) {
			found = True;
			labeldata->mnemonic = mnemonic;
		    }
		}

		if (!found) {
		    mnemonicString[1] = labeldata->mnemonic;
		    if(labeldata->text_format == OL_WC_STR_REP) {
			wchar_t ws[4];
		    
			(void)mbstowcs(ws, (char *)mnemonicString, 4);
			*mnemonicWidth = 
			    XwcTextEscapement((XFontSet)labeldata->font, ws, 3);
		    } else {
			*mnemonicWidth = XmbTextEscapement(
			    (XFontSet)labeldata->font, (char *)mnemonicString,
			    3);
		    }
		}
	    }
	}
    }

    if (labeldata->flags & TL_POPUP) {
	if (labeldata->text_format == OL_WC_STR_REP) {
	    *popupWidth = XwcTextEscapement(labeldata->font, L"...", 3);
	} else {
	    *popupWidth = XmbTextEscapement(labeldata->font, "...", 3);
	}
    } else
	*popupWidth = 0;

    displayStyle = OlQueryAcceleratorDisplay(_OlGetScreenShellOfScreen(scr));
    if (labeldata->accelerator && displayStyle == OL_DISPLAY) {
	if (labeldata->text_format == OL_WC_STR_REP) {
	    int len;
	    wchar_t *ws;

	    len = strlen((char *)labeldata->accelerator) + 1;
	    if (labeldata->qualifier)
		/* make sure ws array is big enough for both strings */
		if ((int)strlen((char *)labeldata->qualifier) >= len)
		    len = strlen((char *)labeldata->qualifier) + 1;
	    ws = (wchar_t *)XtMalloc(sizeof(wchar_t) * len);

	    if (labeldata->qualifier) {
		len = mbstowcs(ws, (char *)labeldata->qualifier,
		    strlen((char *)labeldata->qualifier)+1);
		*qWidth = XwcTextEscapement(labeldata->font, ws, len);
	    } else
		*qWidth = 0;

	    if (labeldata->qualifier || labeldata->meta)
		/* always use widest char, so accelerator fields will line up */
		len = mbstowcs(ws, "M", 2);
	    else
		/* user specified XtNacceleratorText directly, get its width */
		len = mbstowcs(ws, (char *)labeldata->accelerator,
		    strlen((char *)labeldata->accelerator)+1);
	    *accWidth = XwcTextEscapement(labeldata->font, ws, len);
	    XtFree((char *)ws);
	} else {
	    if (labeldata->qualifier)
		*qWidth = XmbTextEscapement(labeldata->font,
		    (char *)labeldata->qualifier,
		    (int)strlen((char *)labeldata->qualifier));
	    else
		*qWidth = 0;

	    if (labeldata->qualifier || labeldata->meta)
		/* always use widest char, so accelerator fields will line up */
		*accWidth = XmbTextEscapement(labeldata->font, "M", 1);
	    else
		/* user specified XtNacceleratorText directly, get its width */
		*accWidth = XmbTextEscapement(labeldata->font,
		    (char *)labeldata->accelerator,
		    (int)strlen((char *)labeldata->accelerator));
	}
    } else {
	*qWidth = 0;
	if (displayStyle == OL_DISPLAY && 
		(labeldata->flags & TL_LAYOUT_WITH_META)) {
	    if (labeldata->text_format == OL_WC_STR_REP) {
		int len;
		wchar_t ws[2];

		len = mbstowcs(ws, "M", 2);
		*accWidth = XwcTextEscapement(labeldata->font, ws, len);
	    } else
		*accWidth = XmbTextEscapement(labeldata->font, "M", 1);
	} else
	    *accWidth = 0;
    }
}

/* Draw a text label in the given button. */

void
OlgxDrawTextButton(Screen *scr, Drawable win, OlgxAttrs *pInfo, 
	Position x, Position y, Dimension buttonWidth, Dimension buttonHeight, 
	XtPointer lbldata, OlDefine buttonType, int olgxFlags)
{
    OlgxTextLbl		*labeldata = (OlgxTextLbl *)lbldata;
    Position		xLbl;			/* position of main label */
    Dimension		lblWidth, lblHeight;	/* label dimensions in pixels */
    Dimension		mnemonicWidth, popupWidth, totalWidth;
    Dimension		qWidth, metaWidth, accWidth, borderWidth;
    Dimension		padWidth = ButtonSpace_Width(pInfo->ginfo);
    Dimension		useWidth;	/* usable area of buttonWidth */
    int			lblLen;		/* length of string in chars */
    OlDefine		displayStyle;
    XRectangle		overall_ink_extents, overall_logical_extents;
    Underlinelabel	underlineLabel;
    XtPointer		olgxLabel = NULL;
    XtPointer		mainLabel, qLabel = NULL, accLabel = NULL;
    int			markType = OLGX_NORMAL;
    int			qPos = 0, markPos = 0, accPos = 0, metaPos;
    void		(*olgxDrawProc)();
    Boolean		freeQ = False, freeAcc = False, drawAcc;
    int			lblFlags = labeldata->flags; /* may want to change this,
					              * so make private copy */

    /* Get the size of the label, mnemonic, ellipses, and accelerator, if any */
    switch(labeldata->text_format) {
      case OL_SB_STR_REP:
	sizeLabelParts(scr, labeldata, &lblWidth, &mnemonicWidth, &popupWidth, 
	    &qWidth, &accWidth, &lblHeight);
	break;
      case OL_MB_STR_REP:
	MbsizeLabelParts(scr, labeldata, &lblWidth, &mnemonicWidth, &popupWidth,
	    &qWidth, &accWidth, &lblHeight);
	XmbTextExtents((XFontSet)labeldata->font, 
		(char *)labeldata->label, 
		strlen ((char *)labeldata->label), 
		&overall_ink_extents, 
		&overall_logical_extents);
	break;
      case OL_WC_STR_REP:
	MbsizeLabelParts(scr, labeldata, &lblWidth, &mnemonicWidth, &popupWidth,
	    &qWidth, &accWidth, &lblHeight);
	XwcTextExtents((XFontSet)labeldata->font, 
		(wchar_t *)labeldata->label, 
		wslen ((wchar_t *)labeldata->label), 
		&overall_ink_extents, 
		&overall_logical_extents);
	olgxFlags |= OLGX_LABEL_IS_WCS;
	break;
    } /* switch */

    switch (buttonType) { 
        case OL_LABEL: 
            olgxDrawProc = olgx_draw_accel_label;
	    borderWidth = 0;
	    /* y gives top left of button object, so if we are not actually
	     * drawing a button, just its label, we need to adjust the y value
	     * for the text.
	     */
	    if (labeldata->text_format == OL_SB_STR_REP)
		y = y + (int)(buttonHeight - lblHeight)/2 +
		    ((XFontStruct *)labeldata->font)->ascent;
	    else
		y = y + (int)(buttonHeight - lblHeight)/2 + 
		    (-overall_logical_extents.y);
            break;
        case OL_RECTBUTTON:
            olgxDrawProc = olgx_draw_accel_choice_item; 
	    borderWidth = ButtonSpace_Width(pInfo->ginfo);
            break;
        case OL_OBLONG:
        case OL_BUTTONSTACK:  
        default: 
            olgxDrawProc = olgx_draw_accel_button; 
	    borderWidth = ButtonEndcap_Width(pInfo->ginfo);
            break;
    }

    useWidth = buttonWidth - 2*borderWidth;
    totalWidth = lblWidth + mnemonicWidth + popupWidth;
    if (OlQueryAcceleratorDisplay(_OlGetScreenShellOfScreen(scr))==OL_DISPLAY) {
	if (labeldata->qualifier)
	    totalWidth += padWidth + qWidth;
	if (labeldata->meta || (lblFlags & TL_LAYOUT_WITH_META))
	    totalWidth += padWidth * 2;
	if (labeldata->accelerator) {
	    totalWidth += padWidth + accWidth;
	    drawAcc = True;
	} else
	    drawAcc = False;
    } else {
	drawAcc = False;
	lblFlags &= ~TL_LAYOUT_WITH_META;
    }
    if (totalWidth > useWidth) {
	/* Label is too wide.  Do not draw the accelerator. */
	totalWidth = lblWidth + mnemonicWidth + popupWidth;
	drawAcc = False;
    }

    /* Position the label */
    if (labeldata->justification == TL_LEFT_JUSTIFY || totalWidth > useWidth)
	xLbl = x + borderWidth;
    else {
	if (labeldata->justification == TL_CENTER_JUSTIFY)
	    xLbl = x + (int) (buttonWidth - totalWidth) / 2;
	else
	    xLbl = x + buttonWidth - borderWidth - totalWidth;
    }

    if (labeldata->text_format != OL_WC_STR_REP)
	lblLen = strlen((char *)labeldata->label);
    else
	lblLen = wslen((wchar_t *)labeldata->label);

    /* Position the mnemonic, if there is one and there's room */
    displayStyle = OlQueryMnemonicDisplay(_OlGetScreenShellOfScreen(scr));
    if (displayStyle == OL_DISPLAY)
	displayStyle = OL_UNDERLINE;
    if (labeldata->mnemonic && totalWidth <= useWidth &&
	(displayStyle == OL_HIGHLIGHT || displayStyle == OL_UNDERLINE)) {

	OlStr		pMnemonic = NULL;
	wchar_t 	wmnemonic;

	(void)mbtowc(&wmnemonic, (const char *)&labeldata->mnemonic, 1);

	pMnemonic = ( (labeldata->text_format == OL_WC_STR_REP) ?
	    (OlStr)wschr((wchar_t *)labeldata->label, (int)wmnemonic):
	    (OlStr)strchr((const char *)labeldata->label, 
		(int)labeldata->mnemonic));

	if (pMnemonic) {
	    /* The mnemonic is included in the label */
	    if(labeldata->text_format != OL_WC_STR_REP)
	    	underlineLabel.position = 
		    (char *)pMnemonic - (char *)labeldata->label;
	    else
	    	underlineLabel.position = 
		    (wchar_t *)pMnemonic - (wchar_t *)labeldata->label;
	    underlineLabel.label = labeldata->label;

	} else {
	    /* The mnemonic is not part of the label. */
	    mnemonicString[1] = labeldata->mnemonic;

	    if (labeldata->text_format != OL_WC_STR_REP) {
		olgxLabel = (XtPointer)XtMalloc(lblLen + 4);
		strcpy(olgxLabel, (char *)labeldata->label);
		strcat(olgxLabel, (char *)mnemonicString);
	    } else {
		wchar_t ws[4];

		(void)mbstowcs(ws, (char *)mnemonicString, 4);
		olgxLabel = (XtPointer)XtMalloc((lblLen + 4) * sizeof(wchar_t));
		wscpy((wchar_t *)olgxLabel, (wchar_t *)labeldata->label);
		wscat((wchar_t *)olgxLabel, ws);
	    }
	    underlineLabel.position = lblLen + 1;
	    underlineLabel.label = olgxLabel;
	}
	mainLabel = (XtPointer)&underlineLabel;

	/* Either underline it or reverse the fg and bg */
	if (displayStyle == OL_HIGHLIGHT)
	    olgxFlags |= OLGX_LABEL_HAS_HIGHLIGHT;
	else
	    olgxFlags |= OLGX_LABEL_HAS_UNDERLINE;
    } else
	mainLabel = (XtPointer)labeldata->label;

    /* If the label will be truncated, draw the arrow. If not, add
     * the ellipses, if any.
     */
    if ((Dimension) (lblWidth+popupWidth) > useWidth) {
	olgxFlags |= OLGX_MORE_ARROW;
	/* if this label has a menu mark, don't draw it, there isn't room */
	olgxFlags &= ~OLGX_MENU_MARK;
    } else if (lblFlags & TL_POPUP) {
	if (labeldata->text_format != OL_WC_STR_REP) {
	    if (olgxLabel) {
		olgxLabel = 
		    (XtPointer)XtRealloc(olgxLabel, strlen(olgxLabel) + 4);
		underlineLabel.label = olgxLabel;
	    } else {
		olgxLabel = (XtPointer)XtMalloc(lblLen + 4);
		strcpy(olgxLabel, (char *)labeldata->label);
		if (mainLabel == (XtPointer)&underlineLabel)
		    underlineLabel.label = olgxLabel;
		else
		    mainLabel = olgxLabel;
	    }
	    strcat(olgxLabel, "...");
	} else {
	    if (olgxLabel) {
		olgxLabel = (XtPointer)XtRealloc(olgxLabel, 
		    (wslen((wchar_t *)olgxLabel) + 4) * sizeof(wchar_t));
		underlineLabel.label = olgxLabel;
	    } else {
		olgxLabel = (XtPointer)XtMalloc((lblLen + 4) * sizeof(wchar_t));
		wscpy((wchar_t *)olgxLabel, (wchar_t *)labeldata->label);
		if (mainLabel == (XtPointer)&underlineLabel)
		    underlineLabel.label = olgxLabel;
		else
		    mainLabel = olgxLabel;
	    }
	    wscat((wchar_t *)olgxLabel, L"...");
	}
    }

    metaWidth = (labeldata->meta || (lblFlags & TL_LAYOUT_WITH_META)) ? 
	    padWidth * 2 : 0;
    metaPos = (labeldata->justification != TL_CENTER_JUSTIFY) ?
	x + buttonWidth - borderWidth - accWidth - metaWidth :
	xLbl + totalWidth - accWidth - metaWidth;
    if (olgxFlags & OLGX_MENU_MARK) {
	markType = (olgxFlags & OLGX_VERT_MENU_MARK) ? 
	    OLGX_VERT_MENU_MARK : OLGX_HORIZ_MENU_MARK;
	markPos = (lblFlags & TL_LAYOUT_WITH_META) ?
	    metaPos : x + buttonWidth - borderWidth - padWidth;
	qLabel = accLabel = NULL;
    } else if (drawAcc) {
	/* Set up accelerator labels and positions */
	if (labeldata->qualifier) {
	    if (labeldata->text_format != OL_WC_STR_REP)
		qLabel = (XtPointer)labeldata->qualifier;
	    else {
	        qLabel = (XtPointer)XtMalloc(sizeof(wchar_t) * 
		    (strlen((char *)labeldata->qualifier) + 1) );
		(void)mbstowcs((wchar_t *)qLabel, (char *)labeldata->qualifier,
		    strlen((char *)labeldata->qualifier)+1 );
		freeQ = True;
	    }
	    qPos = metaPos - padWidth - qWidth;
	}
	if (labeldata->meta) {
	    markType = OLGX_DIAMOND_MARK;
	    markPos = metaPos;
	}
	if (labeldata->text_format != OL_WC_STR_REP)
	    accLabel = (XtPointer)labeldata->accelerator;
	else {
	    accLabel = (XtPointer)XtMalloc(sizeof(wchar_t) *
		(strlen((char *)labeldata->accelerator) + 1) );
	    (void)mbstowcs((wchar_t *)accLabel, (char *)labeldata->accelerator,
		strlen((char *)labeldata->accelerator)+1 );
	    freeAcc = True;
	}
	accPos = metaPos + metaWidth;
    }

    (*olgxDrawProc)(pInfo->ginfo, win, x, y, buttonWidth, buttonHeight,
	mainLabel, xLbl, qLabel, qPos, markType, markPos, accLabel, accPos,
	NULL, olgxFlags);
    if (olgxLabel)
	XtFree(olgxLabel);
    if (freeQ)
	XtFree(qLabel);
    if (freeAcc)
	XtFree(accLabel);
}

/* Calculate size of text label.  Size is the bounding box of the text. */

void
OlgxSizeTextLabel(Screen *scr, OlgxAttrs *pInfo, XtPointer lbldata, Dimension *pWidth, Dimension *pHeight)
{
    register OlgxTextLbl *labeldata = (OlgxTextLbl *)lbldata;
    Dimension	labelWidth, mnemonicWidth, popupWidth, qWidth, accWidth;
    Dimension	padWidth = ButtonSpace_Width(pInfo->ginfo);

    if (labeldata->text_format == OL_SB_STR_REP) {
	sizeLabelParts (scr, labeldata, &labelWidth, &mnemonicWidth, 
	    &popupWidth, &qWidth, &accWidth, 
	    pHeight);
    } else if (labeldata->text_format == OL_WC_STR_REP) {
	MbsizeLabelParts (scr, labeldata, &labelWidth, &mnemonicWidth, 
	    &popupWidth, &qWidth, &accWidth, 
	    pHeight);
    } else {
	MbsizeLabelParts (scr, labeldata, &labelWidth, &mnemonicWidth, 
	    &popupWidth, &qWidth, &accWidth, 
	    pHeight);
    }

    *pWidth = labelWidth + mnemonicWidth + popupWidth;
    if (OlQueryAcceleratorDisplay(_OlGetScreenShellOfScreen(scr))==OL_DISPLAY) {
	if (labeldata->qualifier)
	    *pWidth += padWidth + qWidth;
	if (labeldata->meta || (labeldata->flags & TL_LAYOUT_WITH_META))
	    *pWidth += padWidth * 2;
	if (labeldata->accelerator)
	    *pWidth += padWidth + accWidth;
    }
}

/* Draw a pixmap label in the given box.
 * The flags member of the labeldata structure contains additional information
 * on how to draw the label.
 * Label images can be pixmaps, bitmaps, or X Images, indicated by the type
 * member.
 */

void
OlgxDrawImageButton(Screen *scr, Drawable win, OlgxAttrs *pInfo,
	Position x, Position y, Dimension buttonWidth, Dimension buttonHeight, 
	XtPointer lbldata, OlDefine buttonType, int olgxFlags)
{
    OlgxPixmapLbl *labeldata = (OlgxPixmapLbl *)lbldata;
    Position	xLbl;			/* position in window to draw label */
    Dimension	lblWidth, lblHeight;	/* desired dimensions of label */
    Dimension	borderWidth;
    Dimension   padWidth = ButtonSpace_Width(pInfo->ginfo);
    XtPointer	mainLabel;
    Pixlabel	pixLabel;
    Ximlabel	ximLabel;
    void	(*olgxDrawProc)();
    int		markType = OLGX_NORMAL, markPos = 0;

    OlgxSizePixmapLabel(scr, pInfo, (XtPointer)labeldata, &lblWidth, &lblHeight);
    if (labeldata->type == PL_IMAGE) {
	ximLabel.ximage = labeldata->label.image;
	ximLabel.width = lblWidth;
	ximLabel.height = lblHeight;
	mainLabel = (XtPointer)&ximLabel;
    } else {
	pixLabel.pixmap = labeldata->label.pixmap;
	pixLabel.width = lblWidth;
	pixLabel.height = lblHeight;
	mainLabel = (XtPointer)&pixLabel;
    }

    switch (buttonType) { 
        case OL_LABEL: 
            olgxDrawProc = olgx_draw_accel_label; 
	    borderWidth = 0;
            /* y gives top left of button object, so if we are not actually
             * drawing a button, just its label, we need to adjust the y value
             * for the label, to centre it vertically.
             */
	    y = y + (int)(buttonHeight - lblHeight)/2 + lblHeight;
            break;
        case OL_RECTBUTTON:
            olgxDrawProc = olgx_draw_accel_choice_item; 
	    borderWidth = ButtonSpace_Width(pInfo->ginfo);
            break;
        case OL_OBLONG:
        case OL_BUTTONSTACK:  
        default: 
            olgxDrawProc = olgx_draw_accel_button; 
	    borderWidth = ButtonEndcap_Width(pInfo->ginfo);
            break;
    }

    /* If the label is not tiled, then figure out where to draw it and put
     * it there.
     */
    if (!(labeldata->flags & PL_TILED)) {
	switch (labeldata->justification) {
	  case TL_LEFT_JUSTIFY:
	    xLbl = x + borderWidth;
	    break;

	  case TL_RIGHT_JUSTIFY:
	    xLbl = x + (Position) (buttonWidth - lblWidth - borderWidth);
	    break;

	  default:
	  case TL_CENTER_JUSTIFY:
	    xLbl = x + (Position) (buttonWidth - lblWidth) / 2;
	    break;
	}
    } else {	/* label is tiled */
	/* OLGX_TODO: how to specify tiling to OLGX? */
    }

    if (olgxFlags & OLGX_MENU_MARK) {
	markPos = xLbl + lblWidth + padWidth;
	if (markPos >= (int)(x + buttonWidth - borderWidth)) {
	    markPos = 0;
	    /* don't draw menu mark, there isn't room */
	    olgxFlags &= ~OLGX_MENU_MARK;
	} else
	    markType = (olgxFlags & OLGX_VERT_MENU_MARK) ?
		OLGX_VERT_MENU_MARK : OLGX_HORIZ_MENU_MARK;
    }

    (*olgxDrawProc)(pInfo->ginfo, win, x, y, buttonWidth, buttonHeight,
        mainLabel, xLbl, NULL, 0, markType, markPos, NULL, 0, NULL, olgxFlags);
}

/* Calculate size of an image label. */

void
OlgxSizePixmapLabel(Screen *scr, OlgxAttrs *pInfo, XtPointer lbldata,
		    Dimension *pWidth, Dimension *pHeight)
{
    OlgxPixmapLbl *labeldata = (OlgxPixmapLbl *)lbldata;
    unsigned	pix_width, pix_height;
    unsigned	ignore_value;
    Drawable	ignore_window;

    switch (labeldata->type) {
    case PL_IMAGE:
	*pWidth = labeldata->label.image->width;
	*pHeight = labeldata->label.image->height;
	break;

    case PL_PIXMAP:
    case PL_BITMAP:
	(void) XGetGeometry(DisplayOfScreen (scr), labeldata->label.pixmap,
			    &ignore_window, (int *) &ignore_value,
			    (int *) &ignore_value,
			    &pix_width, &pix_height, &ignore_value,
			    &ignore_value);
	*pWidth = pix_width;
	*pHeight = pix_height;
	break;
    }
}
