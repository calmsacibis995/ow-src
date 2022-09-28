#pragma ident	"@(#)OlStrMthds.c	302.8	97/03/26 lib/libXol SMI"       /* OLIT */
 
/*
 *      Copyright (C) 1986,1991  Sun Microsystems, Inc
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
 
#include <libintl.h>
#include <stdlib.h>
#include <widec.h>
#include <stdarg.h>

#include <Xol/OlStrMthdsI.h>
#include <Xol/OpenLookP.h>
#include <Xol/RootShell.h>


static OlStr		SB_EmptyString(void);
static int		SB_NumBytes(OlStr s);
static int		SB_NumUnits(OlStr s);
static int		SB_NumChars(OlStr s);
static OlStr 
SB_SubString(
	OlStr s,
	int nthchar);
static OlStr		SB_Cpy(OlStr dest, OlStr src);
static OlStr		SB_NCpy(OlStr dest, OlStr src, int nchars);
static OlStr		SB_Cat(OlStr dest, OlStr src);
static OlStr		SB_NCat(OlStr dest, OlStr src, int nchars);
static int		SB_Cmp(OlStr s1, OlStr s2);
static int		SB_NCmp(OlStr s1, OlStr s2, int nchars);
static int 
SB_Lookup(
	XIC ic,
	XKeyEvent*  event,
	OlStr s,
	int nunits,
	KeySym*  keysym,
	Status*  status_ret);
static void 
SB_Draw(
	Display*  dpy,
	Drawable d,
	OlFont fs,
	GC gc,
	int x,
	int y,
	OlStr s,
	int nunits);
static void 
SB_DrawImage(
	Display*  dpy,
	Drawable d,
	OlFont fs,
	GC gc,
	int x,
	int y,
	OlStr s,
	int nunits);
static int 
SB_Width(
	OlFont fs,
	OlStr s,
	int nunits);
static int 
SB_Extents(
	OlFont fs,
	OlStr s,
	int nunits,
	XRectangle*  overall_ink,
	XRectangle*  overall_logical);
static int		SB_Printf(OlStr s, const char *, ...);
static int		SB_Atoi(OlStr s);
static double		SB_Atof(OlStr s);
static int		MB_NumChars(OlStr s);
static OlStr 
MB_SubString(
	OlStr s,
	int nthchar);
static OlStr		MB_NCpy(OlStr dest, OlStr src, int nchars);
static OlStr		MB_NCat(OlStr dest, OlStr src, int nchars);
static int		MB_NCmp(OlStr s1, OlStr s2, int nchars);
static int 
MB_Lookup(
	XIC ic,
	XKeyEvent*  event,
	OlStr s,
	int nunits,
	KeySym*  keysym,
	Status*  status_ret);
static void 
MB_Draw(
	Display*  dpy,
	Drawable d,
	OlFont fs,
	GC gc,
	int x,
	int y,
	OlStr s,
	int nunits);
static void 
MB_DrawImage(
	Display*  dpy,
	Drawable d,
	OlFont fs,
	GC gc,
	int x,
	int y,
	OlStr s,
	int nunits);
static int 
MB_Width(
	OlFont fs,
	OlStr s,
	int nunits);
static int 
MB_Extents(
	OlFont fs,
	OlStr s,
	int nunits,
	XRectangle*  overall_ink,
	XRectangle*  overall_logical);
static char*		MB_ToCT(Display*  display, OlStr s, int* ct_len);
static OlStr*		MB_FromCT(Display*  display, char* ct_string);
static void		MB_FreeList(OlStr* str_list);

static OlStr		WC_EmptyString(void);
static int		WC_NumBytes(OlStr s);
static int		WC_NumUnits(OlStr s);
static int		WC_NumChars(OlStr s);

static OlStr 
WC_SubString(
	OlStr s,
	int nthchar);

static OlStr		WC_Cpy(OlStr dest, OlStr src);
static OlStr		WC_NCpy(OlStr dest, OlStr src, int nchars);
static OlStr		WC_Cat(OlStr dest, OlStr src);
static OlStr		WC_NCat(OlStr dest, OlStr src, int nchars);
static int		WC_Cmp(OlStr s1, OlStr s2);
static int		WC_NCmp(OlStr s1, OlStr s2, int nchars);
static int 
WC_Lookup(
	XIC ic,
	XKeyEvent*  event,
	OlStr s,
	int nunits,
	KeySym*  keysym,
	Status*  status_ret);
static void 
WC_Draw(
	Display*  dpy,
	Drawable d,
	OlFont fs,
	GC gc,
	int x,
	int y,
	OlStr s,
	int nunits);
static void 
WC_DrawImage(
	Display*  dpy,
	Drawable d,
	OlFont fs,
	GC gc,
	int x,
	int y,
	OlStr s,
	int nunits);
static int 
WC_Width(
	OlFont fs,
	OlStr s,
	int nunits);
static int 
WC_Extents(
	OlFont fs,
	OlStr s,
	int nunits,
	XRectangle*  overall_ink,
	XRectangle*  overall_logical);
static char*		WC_ToCT(Display*  display, OlStr s, int* ct_len);
static OlStr*		WC_FromCT(Display*  display, char* ct_string);
static void		WC_FreeList(OlStr* str_list);
static int		WC_Printf(OlStr s, const char *, ...);
static int		WC_Atoi(OlStr s);
static double		WC_Atof(OlStr s);


OlStrMethods str_methods[NUM_SUPPORTED_REPS] = {
    {					/* OL_SB_STR_REP */
        /* string */
		SB_EmptyString, /* StrEmptyString */
                SB_NumBytes,	/* StrNumBytes */
                SB_NumUnits,	/* StrNumUnits */
                SB_NumChars,	/* StrNumChars */
                SB_SubString,	/* StrSubString */
        /* inter string operations */
                SB_Cpy,		/* StrCpy */
                SB_NCpy,	/* StrNCpy */
                SB_Cat,		/* StrCat */
                SB_NCat,	/* StrNCat */
                SB_Cmp,		/* StrCmp */
                SB_NCmp,	/* StrNCmp */
                /* maybe add more here */
        /* text operations */
                SB_Lookup,	/* StrLookup */
                SB_Draw,	/* StrDraw */
                SB_DrawImage,	/* StrDrawImage */
		SB_Width,       /* StrWidth */
                SB_Extents,	/* StrExtents */
                MB_ToCT,	/* StrToCT: same as MB */
                MB_FromCT,	/* StrFromCT: same as MB */
		MB_FreeList,	/* StrFreeList */
	/* printf operations */
		SB_Printf,	/* StrPrintf */
	/* String-to-Int operations */
		SB_Atoi,	/* StrAtoi */
		SB_Atof,	/* StrAtof */
    },
    {					/* OL_MB_STR_REP */
        /* string */
		SB_EmptyString, /* StrEmptyString */
                SB_NumBytes,	/* StrNumBytes */
                SB_NumUnits,	/* StrNumUnits */
                MB_NumChars,	/* StrNumChars */
                MB_SubString,	/* StrSubString */
        /* inter string operations */
                SB_Cpy,		/* StrCpy */
                MB_NCpy,	/* StrNCpy */
                SB_Cat,		/* StrCat */
                MB_NCat,	/* StrNCat */
                SB_Cmp,		/* StrCmp */
                MB_NCmp,	/* StrNCmp */
                /* maybe add more here */
        /* text operations */
                MB_Lookup,	/* StrLookup */
                MB_Draw,	/* StrDraw */
                MB_DrawImage,	/* StrDrawImage */
		MB_Width,       /* StrWidth */
                MB_Extents,	/* StrExtents */
        /* conversion between rep type and compound text */
                MB_ToCT,	/* StrToCT */
                MB_FromCT,	/* StrFromCT */
		MB_FreeList,	/* StrFreeList */
	/* printf operations */
		SB_Printf,	/* StrPrintf */
	/* String-to-Int operations */
		SB_Atoi,	/* StrAtoi */
		SB_Atof,	/* StrAtof */
    },
    {					/* OL_WC_STR_REP */
        /* string */
		WC_EmptyString, /* StrEmptyString */
                WC_NumBytes,	/* StrNumBytes */
                WC_NumUnits,	/* StrNumUnits */
                WC_NumChars,	/* StrNumChars */
                WC_SubString,	/* StrSubString */
        /* inter string operations */
                WC_Cpy,		/* StrCpy */
                WC_NCpy,	/* StrNCpy */
                WC_Cat,		/* StrCat */
                WC_NCat,	/* StrNCat */
                WC_Cmp,		/* StrCmp */
                WC_NCmp,	/* StrNCmp */
                /* maybe add more here */
        /* text operations */
                WC_Lookup,	/* StrLookup */
                WC_Draw,	/* StrDraw */
                WC_DrawImage,	/* StrDrawImage */
		WC_Width,       /* StrWidth */
                WC_Extents,	/* StrExtents */
        /* conversion between rep type and compound text */
                WC_ToCT,	/* StrToCT */
                WC_FromCT,	/* StrFromCT */
		WC_FreeList,	/* StrFreeList */
	/* printf operations */
		WC_Printf,	/* StrPrintf */
	/* String-to-Int operations */
		WC_Atoi,	/* StrAtoi */
		WC_Atof,	/* StrAtof */
    },
};


/*
 ****************************************************************************
 * OL_SB_STR_REP (SINGLE BYTE equivalent to ISO-Latin1)
 ****************************************************************************
 */

/*
 *----------------------------------------
 *  GROUP: string
 *----------------------------------------
 */
static OlStr
SB_EmptyString(void)
{
    static char * s = "";

    return((OlStr)s);
}

static int
SB_NumBytes(OlStr s)
{
    return((int)(strlen((char *)s) + sizeof(char)));
}

static int
SB_NumUnits(OlStr s)
{
    return(strlen((char *)s));
}

static int
SB_NumChars(OlStr s)
{
    return(strlen((char *)s));
}

static OlStr
SB_SubString(
    OlStr	s,
    int		nthchar)
{
    return((OlStr)&( ((char *)s)[nthchar] ));
}

/*
 *----------------------------------------
 *  GROUP: inter string operations
 *----------------------------------------
 */

static OlStr
SB_Cpy(OlStr dest, OlStr src)
{
    return((OlStr)strcpy((char *)dest, (char *)src));
}

static OlStr
SB_NCpy(OlStr dest, OlStr src, int nchars)
{
    return((OlStr)strncpy((char *)dest, (char *)src, nchars));
}

static OlStr
SB_Cat(OlStr dest, OlStr src)
{
    return((OlStr)strcat((char *)dest, (char *)src));
}

static OlStr
SB_NCat(OlStr dest, OlStr src, int nchars)
{
    return((OlStr)strncat((char *)dest, (char *)src, nchars));
}

static int
SB_Cmp(OlStr s1, OlStr s2)
{
    return(strcmp((char *)s1, (char *)s2));
}

static int
SB_NCmp(OlStr s1, OlStr s2, int nchars)
{
    return(strncmp((char *)s1, (char *)s2, nchars));
}

/*
 *----------------------------------------
 * maybe add more here
 *----------------------------------------
 */


/*
 *----------------------------------------
 *  GROUP: text operations
 *----------------------------------------
 */

static int
SB_Lookup(
    XIC		ic,
    XKeyEvent *	event,
    OlStr	s,
    int		nunits,
    KeySym *	keysym,
    Status *	status_ret)
{
    return(XLookupString(event, (char *)s, nunits, keysym,
			   (XComposeStatus *)status_ret));
}

static void
SB_Draw(
    Display *	dpy,
    Drawable	d,
    OlFont  	fs,
    GC		gc,
    int		x,
    int		y,
    OlStr	s,
    int		nunits)
{
    XDrawString(dpy, d, gc, x, y, (char *)s, nunits);
}

static void
SB_DrawImage(
    Display *	dpy,
    Drawable	d,
    OlFont  	fs,
    GC		gc,
    int		x,
    int		y,
    OlStr	s,
    int		nunits)
{
    XDrawImageString(dpy, d, gc, x, y, (char *)s, nunits);
}

static int
SB_Width(
    OlFont  	fs,
    OlStr	s,
    int		nunits)
{
    return(XTextWidth((XFontStruct *)fs, (char *)s, nunits));
}

static int
SB_Extents(
    OlFont  	fs,
    OlStr	s,
    int		nunits,
    XRectangle *overall_ink,
    XRectangle *overall_logical)
{
    int		ret_val;
    int		dir;
    int		asc;
    int		desc;
    XCharStruct	overall;

    ret_val = XTextExtents((XFontStruct *)fs, (char *)s, nunits,
			    &dir, &asc, &desc, &overall);
    overall_ink->width = overall.width;
    overall_ink->height = asc+desc;
    overall_ink->x = overall.lbearing;
    overall_ink->y = -asc;
    *overall_logical = *overall_ink;
    overall_logical->x = 0;
    return(ret_val);
}

/*
 *----------------------------------------
 *  GROUP: conversion between rep type and compound text
 *----------------------------------------
 */


/*
SB_FreeList(OlStr *str_list)
    is MB_FreeList
*/

/*
 *----------------------------------------
 *  GROUP: Printf functions
 *----------------------------------------
 */

static int
SB_Printf(OlStr s, const char * format, ...)
{
	va_list args;
	int i;

	va_start(args, format);
	i = vsprintf((char *)s, format, args);
	va_end(args);

	return i;
}

/*
 *----------------------------------------
 *  GROUP: String-to-Number conversions 
 *----------------------------------------
 */
static int 
SB_Atoi(OlStr s)
{
	return (atoi((char *)s));
}

static double
SB_Atof(OlStr s)
{
	return (atof((char *)s));
}


/*
 ****************************************************************************
 * OL_MB_STR_REP (MULTI BYTE)
 ****************************************************************************
 */

/*
 *----------------------------------------
 *  GROUP: string
 *----------------------------------------
 */

/*
MB_NumBytes(OlStr s) is SB_NumBytes
*/

/*
MB_NumUnits(OlStr s) is SB_NumUnits
*/

static int
MB_NumChars(OlStr s)
{
    register int	l = -1;

    if (s != (OlStr)NULL) {
	register int	j;
	register char *	p = (char *)s;

	for ( l = 0; (j = mblen(p, MB_CUR_MAX)) > 0; l++, p += j )
	    ;
    /* assert(j <= 0) */
	if (j)	/* j < 0: bad char */
	    l = j;
    }
    return(l);
}

static OlStr
MB_SubString(
    OlStr	s,
    int		nthchar)
{
    register char *	p = (char *)s;

    if (p != NULL) {
	register int	j;
	register int	l;

	for ( l = 0; (j = mblen(p, MB_CUR_MAX)) > 0 && l < nthchar;
		l++, p += j )
	    ;
    /* assert(j <= 0 || l == nthchar) */
	if (j < 0)	/* bad char */
	    p = NULL;
    }
    return((OlStr)p);
}
/*
 *----------------------------------------
 *  GROUP: inter string operations
 *----------------------------------------
 */

/*
MB_Cpy(OlStr dest, OlStr src) is SB_Cpy
*/

static OlStr
MB_NCpy(OlStr dest, OlStr src, int nchars)
{
    register char *     p = (char *)src;
 
    if (p != NULL) {
        register int    j;
        register int    l;
        register int    b;
 
        for ( b = l = 0; (j = mblen(p, MB_CUR_MAX)) > 0 && l < nchars;
                b += j, p += j, l++ )
            ;
    /* assert(j <= 0 || l == nchars) */
	if (j < 0)	/* bad char */
	    p = NULL;
	else
	    p = strncpy((char *)dest, (char *)src, b);
    }
    return((OlStr)p);
}

/*
MB_Cat(OlStr dest, OlStr src) is SB_Cat
*/

static OlStr
MB_NCat(OlStr dest, OlStr src, int nchars)
{
    register char *     p = (char *)src;
 
    if (p != NULL) {
        register int    j;
        register int    l;
        register int    b;
 
        for ( b = l = 0; (j = mblen(p, MB_CUR_MAX)) > 0 && l < nchars;
                b += j, p += j, l++ )
            ;
    /* assert(j <= 0 || l == nchars) */
	if (j < 0)	/* bad char */
	    p = NULL;
	else
	    p = strncat((char *)dest, (char *)src, b);
    }
    return((OlStr)p);
}

/*
MB_Cmp(OlStr s1, OlStr s2) is SB_Cmp
*/

static int
MB_NCmp(OlStr s1, OlStr s2, int nchars)
{
    register char *     p = (char *)s1;
    register int	r = -1;
 
    if (p != NULL) {
        register int    j;
        register int    l;
        register int    b;
 
        for ( b = l = 0; (j = mblen(p, MB_CUR_MAX)) > 0 && l < nchars;
                b += j, p += j, l++ )
            ;
    /* assert(j <= 0 || l == nchars) */
	if (j < 0)	/* bad char */
	    r = j;
	else
	    r = strncmp((char *)s1, (char *)s2, b);
    }
    return(r);
}
/*
 *----------------------------------------
 * maybe add more here
 *----------------------------------------
 */


/*
 *----------------------------------------
 *  GROUP: text operations
 *----------------------------------------
 */

static int
MB_Lookup(
    XIC		ic,
    XKeyEvent *	event,
    OlStr	s,
    int		nunits,
    KeySym *	keysym,
    Status *	status_ret)
{
    /* May need to refine this for pre-edit style */
    return(XmbLookupString(ic, event, (char *)s, nunits, keysym, status_ret));
}

static void
MB_Draw(
    Display *	dpy,
    Drawable	d,
    OlFont  	fs,
    GC		gc,
    int		x,
    int		y,
    OlStr	s,
    int		nunits)
{
    XmbDrawString(dpy, d, (XFontSet)fs, gc, x, y, (char *)s, nunits);
}

static void
MB_DrawImage(
    Display *	dpy,
    Drawable	d,
    OlFont  	fs,
    GC		gc,
    int		x,
    int		y,
    OlStr	s,
    int		nunits)
{
    XmbDrawImageString(dpy, d, (XFontSet)fs, gc, x, y, (char *)s, nunits);
}

static int
MB_Width(
    OlFont  	fs,
    OlStr	s,
    int		nunits)
{
    return(XmbTextEscapement((XFontSet)fs, (char *)s, nunits));
}

static int
MB_Extents(
    OlFont  	fs,
    OlStr	s,
    int		nunits,
    XRectangle *overall_ink,
    XRectangle *overall_logical)
{
    return(XmbTextExtents((XFontSet)fs, (char *)s, nunits, overall_ink, overall_logical));
}

/*
 *----------------------------------------
 *  GROUP: conversion between rep type and compound text
 *----------------------------------------
 */

static char *
MB_ToCT(Display * display, OlStr s, int * ct_len)
{
    char **		string_list = (char **)&s;
    XTextProperty	text_prop;

    /* convert to CompoundText, but try to leave it as */
    /* string if possible */
    if (XmbTextListToTextProperty(display, string_list, 1,
		XCompoundTextStyle, &text_prop)
	    != Success) {
	OlWarning(dgettext(OlMsgsDomain,
			    "Could not convert to compound text."));
	*ct_len = 0;
	return NULL;
    }
    *ct_len = text_prop.nitems;
    return (char *)text_prop.value;
}


static OlStr *
MB_FromCT(Display * display, char * ct_string)
{
    XTextProperty	text_prop;
    char **		mbstr_list;
    int 		mbstr_count;

    text_prop.encoding = (Atom)OlInternAtom(display,COMPOUND_TEXT_NAME);
    text_prop.value = (unsigned char *)ct_string;
    text_prop.nitems = strlen(ct_string);
    text_prop.format = 8;

    if (XmbTextPropertyToTextList(display, &text_prop,
				    (char ***)&mbstr_list, &mbstr_count)
	    != Success) {
	XFreeStringList(mbstr_list);
	OlWarning(dgettext(OlMsgsDomain,
			    "Could not convert from compound text."));
	return (OlStr *)NULL;
    }
    return (OlStr *)mbstr_list;
}

static void
MB_FreeList(OlStr *str_list)
{
    XFreeStringList((char **)str_list);
}

/*
 ****************************************************************************
 * OL_WC_STR_REP (WIDE CHAR)
 ****************************************************************************
 */

/*************************************************************************
 *
 * WARNING: This rep will work iff you are running SVR4 or
 *          SUN-OS 4.1.1 with Asian L10N (e.g. JLE, KLE)
 *
 *************************************************************************/

/*
 *----------------------------------------
 *  GROUP: string
 *----------------------------------------
 */
static OlStr
WC_EmptyString(void)
{
    static wchar_t * s = L"";

    return((OlStr)s);
}

static int
WC_NumBytes(OlStr s)
{
    return((int)(wslen((wchar_t *)s)*sizeof(wchar_t) + sizeof(wchar_t)));
}

static int
WC_NumUnits(OlStr s)
{
    return(wslen((wchar_t *)s));
}

static int
WC_NumChars(OlStr s)
{
    return(wslen((wchar_t *)s));
}

static OlStr
WC_SubString(
    OlStr	s,
    int		nthchar)
{
    return((OlStr)&( ((wchar_t *)s)[nthchar] ));
}

/*
 *----------------------------------------
 *  GROUP: inter string operations
 *----------------------------------------
 */

static OlStr
WC_Cpy(OlStr dest, OlStr src)
{
    return((OlStr)wscpy((wchar_t *)dest, (wchar_t *)src));
}

static OlStr
WC_NCpy(OlStr dest, OlStr src, int nchars)
{
    return((OlStr)wsncpy((wchar_t *)dest, (wchar_t *)src, nchars));
}

static OlStr
WC_Cat(OlStr dest, OlStr src)
{
    return((OlStr)wscat((wchar_t *)dest, (wchar_t *)src));
}

static OlStr
WC_NCat(OlStr dest, OlStr src, int nchars)
{
    return((OlStr)wsncat((wchar_t *)dest, (wchar_t *)src, nchars));
}

static int
WC_Cmp(OlStr s1, OlStr s2)
{
    return(wscmp((wchar_t *)s1, (wchar_t *)s2));
}

static int
WC_NCmp(OlStr s1, OlStr s2, int nchars)
{
    return(wsncmp((wchar_t *)s1, (wchar_t *)s2, nchars));
}

/*
 *----------------------------------------
 * maybe add more here
 *----------------------------------------
 */


/*
 *----------------------------------------
 *  GROUP: text operations
 *----------------------------------------
 */

static int
WC_Lookup(
    XIC		ic,
    XKeyEvent *	event,
    OlStr	s,
    int		nunits,
    KeySym *	keysym,
    Status *	status_ret)
{
    /* May need to refine this for pre-edit style */
    return(XwcLookupString(ic, event, (wchar_t *)s, nunits,
			    keysym, status_ret));
}

static void
WC_Draw(
    Display *	dpy,
    Drawable	d,
    OlFont  	fs,
    GC		gc,
    int		x,
    int		y,
    OlStr	s,
    int		nunits)
{
    XwcDrawString(dpy, d, (XFontSet)fs, gc, x, y, (wchar_t *)s, nunits);
}

static void
WC_DrawImage(
    Display *	dpy,
    Drawable	d,
    OlFont  	fs,
    GC		gc,
    int		x,
    int		y,
    OlStr	s,
    int		nunits)
{
    XwcDrawImageString(dpy, d, (XFontSet)fs, gc, x, y, (wchar_t *)s, nunits);
}

static int
WC_Width(
    OlFont  	fs,
    OlStr	s,
    int		nunits)
{
    return(XwcTextEscapement((XFontSet)fs, (wchar_t *)s, nunits));
}

static int
WC_Extents(
    OlFont  	fs,
    OlStr	s,
    int		nunits,
    XRectangle *overall_ink,
    XRectangle *overall_logical)
{
    return(XwcTextExtents((XFontSet)fs, (wchar_t *)s, nunits,
			    overall_ink, overall_logical));
}

/*
 *----------------------------------------
 *  GROUP: conversion between rep type and compound text
 *----------------------------------------
 */
 
static char *
WC_ToCT(Display * display, OlStr s, int * ct_len)
{
    wchar_t **          wstring_list = (wchar_t **)&s;
    XTextProperty       text_prop;
 
    if (XwcTextListToTextProperty(display, wstring_list, 1,
                XCompoundTextStyle, &text_prop)
            != Success) {
        OlWarning(dgettext(OlMsgsDomain,
                            "Could not convert to compound text."));
        *ct_len = 0;
        return NULL;
    }
    *ct_len = text_prop.nitems;
    return (char *)text_prop.value;
}
 
static OlStr *
WC_FromCT(Display * display, char * ct_string)
{
    XTextProperty       text_prop;
    wchar_t **          wcstr_list;
    int                wcstr_count;
 
    text_prop.encoding = OlInternAtom(display,COMPOUND_TEXT_NAME);
    text_prop.value = (unsigned char *)ct_string;
    text_prop.nitems = strlen(ct_string);
    text_prop.format = 8;
 
    if (XwcTextPropertyToTextList(display, &text_prop,
                                    (wchar_t ***)&wcstr_list, &wcstr_count)
            != Success) {
        XwcFreeStringList(wcstr_list);
        OlWarning(dgettext(OlMsgsDomain,
                            "Could not convert from compound text."));
        return (OlStr *)NULL;
    }
    return (OlStr *)wcstr_list;
}
 
static void
WC_FreeList(OlStr *str_list)
{
    XwcFreeStringList((wchar_t **)str_list);
}

/*
 *----------------------------------------
 *  GROUP: Printf functions
 *----------------------------------------
 */

/* 
 * Wide char version of vsprintf does'nt exist. So print out as
 * multibyte & convert resulting string to wide-char ...
 */
static int
WC_Printf(OlStr s, const char * format, ...)
{
	va_list args;
	int i;
	/* Sorry .. but we don't know for sure how long the resulting
     * string is gonna be ...
     */  
    static char *mb_buffer = NULL;

	if (!mb_buffer)
		if (!(mb_buffer = malloc(100)))
			return(0);
	

    va_start(args, format);
    i = vsnprintf(mb_buffer, 100, format, args);
    va_end(args);

    /* number of characters printed by vsprintf = i (excluding NULL) */
    i = mbstowcs(s, mb_buffer, i+1);
    return i;
}

/*
 *----------------------------------------
 *  GROUP: String-to-Number conversions 
 *----------------------------------------
 */
static int 
WC_Atoi(OlStr s)
{
	return (watoi((wchar_t *)s));
}

static double
WC_Atof(OlStr s)
{
	return (watof((wchar_t *)s));
}

