#ifndef _FONTS_H
#define _FONTS_H

#pragma ident   "@(#)fonts.h	1.2    93/08/02 SMI"

/**************************************************************************
* Structures
**************************************************************************/

/*
 * The PreviewInfo structure should be defined (more completely) in
 *	preview.h, but it hasn't been checked in yet.
 */
#if 0
typedef struct _PreviewInfo {
        /* Fonts */
        XFontSet        regularFontSet; /* fonts for buttons, menus, etc. */
        XFontSet        boldFontSet;    /* fonts for window title, captions */
        XFontSet        monospaceFontSet; /* fonts for data areas */
} PreviewInfo;
#endif

/**************************************************************************
* Public Functions
**************************************************************************/

extern void	fontsUpdatePreviewInfo(PreviewInfo*);

#endif /* _FONTS_H */
