#ifndef	_XOL_FONTCH_H
#define	_XOL_FONTCH_H

#pragma	ident	"@(#)FontCh.h	1.6	92/12/04 include/Xol SMI"	/* OLIT */

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

/*
 *************************************************************************
 *
 * Description:
 *	This is the "public" include file for the FontChooser Widget
 *
 ******************************file*header********************************
 */

#include <Xol/RubberTile.h>
#include <X11/Intrinsic.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct _OlFCApplyCallbackStruct {
    int		reason;
    String 	current_font_name;
    OlFont	current_font;
} OlFCApplyCallbackStruct;

typedef struct _OlFCRevertCallbackStruct {
    int		reason;
    String 	current_font_name;
    OlFont	current_font;
    String	revert_font_name;
    OlFont	revert_font;
} OlFCRevertCallbackStruct;

typedef OlFCApplyCallbackStruct OlFCCancelCallbackStruct;

typedef struct _OlFCChangedCallbackStruct {
    int		reason;
    String 	current_font_name;
    OlFont	current_font;
    String 	previous_font_name;
    OlFont	previous_font;
} OlFCChangedCallbackStruct;

typedef struct _OlFCErrorCallbackStruct {
    int		reason;
    int		error_num;
    String 	font_name;
} OlFCErrorCallbackStruct;

#define	OL_FC_ERR_NO_FONTS_FOUND		1
#define	OL_FC_ERR_NO_INITIAL_FONT		2
#define	OL_FC_ERR_BAD_INITIAL_FONT		3
#define	OL_FC_ERR_BAD_FONT_SEARCH_SPEC		4
#define	OL_FC_ERR_MISSING_CHARSETS		5
#define	OL_FC_ERR_FONT_UNAVAILABLE		6

    
typedef struct _FontChooserClassRec *	FontChooserWidgetClass;
typedef struct _FontChooserRec *	FontChooserWidget;

extern WidgetClass			fontChooserWidgetClass;

#ifdef	__cplusplus
}
#endif

#endif	/* _XOL_FONTCH_H */
