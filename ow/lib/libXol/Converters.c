#pragma ident	"@(#)Converters.c	302.33	96/06/07 lib/libXol SMI"	/* olmisc:Converters.c 1.14	*/

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

/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <libintl.h>
#include <string.h>

#include <X11/IntrinsicI.h>
#include <X11/StringDefs.h>
#include <X11/CoreP.h>
#include <X11/Xatom.h>

#define XK_LATIN1
#include <X11/keysymdef.h>

#include <Xol/Converters.h>
#include <Xol/DynamicP.h>
#include <Xol/EventObj.h>
#include <Xol/OlI18nP.h>
#include <Xol/OlStrMthdsI.h>
#include <Xol/OpenLookP.h>
#include <Xol/RootShellP.h>
#include <Xol/Util.h>
#include <Xol/OlIm.h>
#include <Xol/TextEdit.h>


/*
 * Things that help us ignore upper/lower case.
 */
#define _XLFD_MAX_LENGTH	(Cardinal)255
#define	CASELESS_STREQU(A,B)	(caseless_strcmp((A), (B)) == 0)
#define LOWER_IT(X) \
	if ((X >= XK_A) && (X <= XK_Z))					\
		X += (XK_a - XK_A);					\
	else if ((X >= XK_Agrave) && (X <= XK_Odiaeresis))		\
		X += (XK_agrave - XK_Agrave);				\
	else if ((X >= XK_Ooblique) && (X <= XK_Thorn))			\
		X += (XK_oslash - XK_Ooblique)

typedef struct Fraction {
	long			numerator;
	long			denominator;
}			Fraction;

#define IsZeroFraction(F) ((F).numerator == 0)
#define IsNegativeFraction(F) ((F).numerator < 0)

typedef struct Units {
	/*
	 * Multiply a number with this modifier (e.g. "1 inch")...
	 */
	const char*		modifier;
	/*
	 * ...by this factor, to get the equivalent number of millimeters.
	 * [The special factor 0 means units are in pixels.]
	 */
	Fraction		factor;
	/*
	 * If this flag is set, the modifier sets the orientation.
	 */
	OlDefine		axis;
}			Units;


/*
 * Local data:
 */

static Units			units[] = {
	{ "i",            254,10,    0 },	/* 25.4 */
	{ "in",           254,10,    0 },	/* 25.4 */
	{ "inch",         254,10,    0 },	/* 25.4 */
	{ "inches",       254,10,    0 },	/* 25.4 */
	{ "m",           1000,1,     0 },
	{ "meter",       1000,1,     0 },
	{ "meters",      1000,1,     0 },
	{ "c",             10,1,     0 },
	{ "cent",          10,1,     0 },
	{ "centimeter",    10,1,     0 },
	{ "centimeters",   10,1,     0 },
	{ "mm",             1,1,     0 },
	{ "millimeter",     1,1,     0 },
	{ "millimeters",    1,1,     0 },
	{ "p",           2540,7227,  0 },	/* 25.4 / 72.27 */
	{ "pt",          2540,7227,  0 },	/* 25.4 / 72.27 */
	{ "point",       2540,7227,  0 },	/* 25.4 / 72.27 */
	{ "points",      2540,7227,  0 },
	{ "pixel",          0,0,     0 },
	{ "pixels",         0,0,     0 },
	{ "hor",            0,0,     OL_HORIZONTAL },
	{ "horz",           0,0,     OL_HORIZONTAL },
	{ "horizontal",     0,0,     OL_HORIZONTAL },
	{ "ver",            0,0,     OL_VERTICAL },
	{ "vert",           0,0,     OL_VERTICAL },
	{ "vertical",       0,0,     OL_VERTICAL },
	{ 0 }
};

typedef struct NameValue {
	String			name;
	int			value;
}			NameValue;

NameValue		gravities[] = {
	{ "forget",      ForgetGravity        },
	{ "northwest",   NorthWestGravity     },
	{ "north",       NorthGravity         },
	{ "northeast",   NorthEastGravity     },
	{ "west",        WestGravity          },
	{ "center",      CenterGravity        },
	{ "east",        EastGravity          },
	{ "southwest",   SouthWestGravity     },
	{ "south",       SouthGravity         },
	{ "southeast",   SouthEastGravity     },
	{ "static",      StaticGravity        },
	{ "unmap",       UnmapGravity         },
	{ "eastwest",    Ol_EastWestGravity   },
	{ "northsouth",  Ol_NorthSouthGravity },
	{ "all",         Ol_AllGravity        },
	{ 0 }
};

NameValue		scales[] = {
	{ "small",	10     	      },
	{ "medium",	12	      },
	{ "large",	14	      },
	{ "extra_large",19	      },
	{ 0 }
};

/*
 * Local routines:
 */

static int (*PrevErrorHandler)(Display* , XErrorEvent*);

static Fraction		NewFraction ( int , int );
static Fraction		StrToFraction ( String , String * );
static Fraction		MulFractions ( Fraction , Fraction );
static int		FractionToInt ( Fraction );
static String           ParseXLFDName(Screen* screen, 
					String str, 
					Boolean resFlag, 
					Boolean psFlag,
					Boolean boldFlag);

static Boolean CvtStringToDimension(
	Display*  display,
	XrmValue*  args,
	Cardinal* num_args,
	XrmValue*  from,
	XrmValue*  to,
	XtPointer*  converter_data
);
static Boolean		CvtStringToPosition (
	Display *		display,
	XrmValue *		args,
	Cardinal *		num_args,
	XrmValue *		from,
	XrmValue *		to,
	XtPointer *		converter_data
);
static Boolean		CvtStringToGravity (
	Display *		display,
	XrmValue *		args,
	Cardinal *		num_args,
	XrmValue *		from,
	XrmValue *		to,
	XtPointer *		converter_data
);
static Boolean		CvtStringToCardinal (
	Display *		display,
	XrmValue *		args,
	Cardinal *		num_args,
	XrmValue *		from,
	XrmValue *		to,
	XtPointer *		converter_data
);
static Boolean		CvtStringToOlDefine (
	Display *		display,
	XrmValue *		args,
	Cardinal *		num_args,
	XrmValue *		from,
	XrmValue *		to,
	XtPointer *		converter_data
);
static Boolean		CvtStringToModifiers (
	Display *		display,
	XrmValue *		args,
	Cardinal *		num_args,
	XrmValue *		from,
	XrmValue *		to,
	XtPointer *		converter_data
);
static Boolean		CvtStringToFont (
	Display *		display,
	XrmValue *		args,
	Cardinal *		num_args,
	XrmValue *		from,
	XrmValue *		to,
	XtPointer *		converter_data
);
static void		FreeFont (
	XtAppContext		app,
	XrmValuePtr		to,
	XtPointer		closure,
	XrmValuePtr		args,
	Cardinal *		num_args
);
static Boolean		CvtStringToFontStruct (
	Display *		display,
	XrmValue *		args,
	Cardinal *		num_args,
	XrmValue *		from,
	XrmValue *		to,
	XtPointer *		converter_data
);
static void		FreeFontStruct (
	XtAppContext		app,
	XrmValuePtr		to,
	XtPointer		closure,
	XrmValuePtr		args,
	Cardinal *		num_args
);
static Boolean          CvtStringToOlFont (
        Display *               display,
        XrmValue *              args,
        Cardinal *              num_args,
        XrmValue *              from,
        XrmValue *              to,
        XtPointer *             converter_data
);
static Boolean          CvtStringToOlFontPrivate (
        Display *               display,
        XrmValue *              args,
        Cardinal *              num_args,
        XrmValue *              from,
        XrmValue *              to,
        XtPointer *             converter_data
);
static Boolean	        CvtStringToChar(
        Display *               display,
        XrmValue *              args,
        Cardinal *              num_args,
        XrmValue *              from,
        XrmValue *              to,
        XtPointer *             converter_data
);

static Boolean          CvtStringToOlImPreeditStyle (
        Display *               display,
        XrmValue *              args,
        Cardinal *              num_args,
        XrmValue *              from,
        XrmValue *              to,
        XtPointer *             converter_data
);
static Boolean		CvtStringToOlImStatusStyle(
	Display *		display, 
	XrmValue *		args, 
	Cardinal *		num_args, 
	XrmValue *		from,
	XrmValue *		to,
	XtPointer *		converter_data
);
static Boolean          CvtLocaleStringToOlStr (
        Display *               display,
        XrmValue *              args,
        Cardinal *              num_args,
        XrmValue *              from,
        XrmValue *              to,
        XtPointer *             converter_data
);
static Boolean          CvtStringToOlStr (
        Display *               display,
        XrmValue *              args,
        Cardinal *              num_args,
        XrmValue *              from,
        XrmValue *              to,
        XtPointer *             converter_data
);
static Boolean          CvtStringToOlStrPrivate (
        Display *               display,
        XrmValue *              args,
        Cardinal *              num_args,
        XrmValue *              from,
        XrmValue *              to,
        XtPointer *             converter_data
);
static Boolean          CvtStringToOlTextSource (
        Display *               display,
        XrmValue *              args,
        Cardinal *              num_args,
        XrmValue *              from,
        XrmValue *              to,
        XtPointer *             converter_data
);
static Boolean          CvtStringToOlEditMode (
        Display *               display,
        XrmValue *              args,
        Cardinal *              num_args,
        XrmValue *              from,
        XrmValue *              to,
        XtPointer *             converter_data
);
static Boolean		CvtStringToOlScale (
	Display *               display,
        XrmValue *              args,
        Cardinal *              num_args,
        XrmValue *              from,
        XrmValue *              to,
        XtPointer *             converter_data
);
static Boolean          CvtStringToFontSet (
        Display *               display,
        XrmValue *              args,
        Cardinal *              num_args,
        XrmValue *              from,
        XrmValue *              to,
        XtPointer *             converter_data
);
static void		FreeOlStr(
	XtAppContext		app,
	XrmValuePtr		to,
	XtPointer		closure,
	XrmValuePtr		args,
	Cardinal *		num_args
);
static void		FreeFontSet(
	XtAppContext		app,
	XrmValuePtr		to,
	XtPointer		closure,
	XrmValuePtr		args,
	Cardinal *		num_args
);
static Boolean		IsScaledInteger (
	String			val,
	int *			pint,
	Screen *		screen
);
static Boolean		IsInteger (
	String			val,
	int *			pint
);
static Boolean		IsFont (
	String			who,
	Screen *		screen,
	String			str,
	Font *			p_font,
	XtPointer *		p_converter_data
);

static String
MapFont(Screen* screen, String str, Boolean resFlag, Boolean psFlag,
	Boolean boldFlag);

static Font		LoadFont (
	Display *		display,
	String			name
);
static Boolean          IsFontSet (
        String                  who,
        Screen *                screen,
        String                  str,
        XFontSet *              p_fontset,
        XtPointer *             p_converter_data
);
static String           MapFontSet (
        Screen *                screen,
        String                  str,
        Boolean                 resFlag,
        Boolean                 psFlag,
	Boolean			boldFlag
);
static XFontSet         LoadFontSet (
        Display *               display,
        String                  name,
	Boolean			missing_charset_ok
);
static int		IgnoreBadName (
	Display *		display,
	XErrorEvent *		event
);
static OlDefine		LookupOlDefine (
	String			name,
	OlDefine	 	value,
	int			flag
);
static Boolean		FetchResource (
	Display *		display,
	String			name,
	String			c_class,
	XrmRepresentation *	p_rep_type,
	XrmValue *		p_value
);
static int		caseless_strcmp (
	register String		s1,
	register String		s2
);
static int		caseless_strncmp (
	register String		s1,
	register String		s2,
	register int		n
);
static int		CompareISOLatin1 (
	String			first,
	String			second,
	register int		count
);
static Boolean		CvtStringToPixel (
	Display *		display,
	XrmValue *		args,
	Cardinal *		num_args,
	XrmValue *		from,
	XrmValue *		to,
	XtPointer *		closure_ret
);
static void		FreePixel (
	XtAppContext		app,
	XrmValuePtr		to,
	XtPointer		closure,
	XrmValuePtr		args,
	Cardinal *		num_args
);
static Boolean		CvtVisualToColormap (
	Display *		display,
	XrmValue *		args,
	Cardinal *		num_args,
	XrmValue *		from,
	XrmValue *		to,
	XtPointer *		closure_ret
);
static void		_OlGetTextFormat(
	Widget w,
	Cardinal *size,
	XrmValue *ret_val
);

static Boolean		GotBadName;

/* Do NOT move these strings to StringList or anywhere outside this file */
static const char XtROlFontPrivate[] = "OlFontPrivate";


XtConvertArgRec convertOlFontArgs[]= {
        {XtBaseOffset,(XtPointer)XtOffsetOf(WidgetRec,core.self),
                        sizeof(Widget)}
};

XtConvertArgRec convertOlFontPrivateArgs[]= {
        {XtWidgetBaseOffset,(XtPointer)XtOffsetOf(WidgetRec,core.screen),
                        sizeof(Screen *)},
        {XtResourceString,(String)XtNtextFormat,sizeof(OlStrRep)}
};

XtConvertArgRec convertOlStrArgs[]= {
	{XtProcedureArg, (XtPointer)_OlGetTextFormat, sizeof(OlStrRep)}
};

/* These below are needed for the Vendor Shell's text_format resource */
XtConvertArgRec vendorOlFontPrivateArgs[]= {
        {XtWidgetBaseOffset, (XtPointer)XtOffsetOf(WidgetRec, core.screen),
                        sizeof(Screen *)},
        {XtProcedureArg, (XtPointer)_OlGetVendorTextFormat, sizeof(OlStrRep)}
};

/* end of vendor args */

XtConvertArgRec convertOlTextSourceArgs[]= {
        {XtResourceString, (String) XtNsourceType, sizeof(String)},
        {XtResourceString, (String) XtNtextFormat, sizeof(OlStrRep)}
};

#define ADD_TO_TABLE	1
#define LOOKUP_VALUE	2

#define HASH_SIZE	127			/* value 2^n - 1	*/
#define	TABLE_SIZE	(HASH_SIZE+1)
#define HASH_QUARK(q)	((int) ((q) & (XrmQuark) HASH_SIZE))

typedef struct OlDefineNode {
	XrmQuark		quark;
	OlDefine		value;
}			OlDefineNode;

typedef struct Bucket {
	OlDefineNode *		array;
	Cardinal		elements;
}			Bucket;

static Bucket		define_table[TABLE_SIZE];

#if	defined(XtSpecificationRelease) && XtSpecificationRelease >= 5 
#define	XtQString	_XtQString
#endif

/**
 ** OlRegisterConverters()
 **/

void
OlRegisterConverters (void)
{
	/*
	 * Look in Xt/Converters.c.
	 */
	static Boolean		registered	= False;


	if (!registered) {
		XtSetTypeConverter (
			XtRString,
			XtRDimension,
			CvtStringToDimension,
			(XtConvertArgList)screenConvertArg,
			1,
			XtCacheByDisplay,
			(XtDestructor)0
		);
		XtSetTypeConverter (
			XtRString,
			XtRPosition,
			CvtStringToPosition,
			(XtConvertArgList)screenConvertArg,
			1,
			XtCacheByDisplay,
			(XtDestructor)0
		);
		XtSetTypeConverter (
			XtRString,
			XtRGravity,
			CvtStringToGravity,
			(XtConvertArgList)0,
			0,
			XtCacheAll,
			(XtDestructor)0
		);
		XtSetTypeConverter (
			XtRString,
			XtRCardinal,
			CvtStringToCardinal,
			(XtConvertArgList)0,
			0,
			XtCacheAll,
			(XtDestructor)0
		);
		XtSetTypeConverter (
			XtRString,
			XtROlDefine,
			CvtStringToOlDefine,
			(XtConvertArgList)0,
			0,
			XtCacheAll,
			(XtDestructor)0
		);
		XtSetTypeConverter (
			XtRString,
			XtRModifiers,
			CvtStringToModifiers,
			(XtConvertArgList)0,
			0,
			XtCacheNone,
			(XtDestructor)0
		);
		XtSetTypeConverter (
			XtRString,
			XtRFont,
			CvtStringToFont,
			(XtConvertArgList)screenConvertArg,
			1,
			XtCacheByDisplay,
			FreeFont
		);
		XtSetTypeConverter (
			XtRString,
			XtRFontStruct,
			CvtStringToFontStruct,
			(XtConvertArgList)screenConvertArg,
			1,
			XtCacheByDisplay,
			FreeFontStruct
		);
		XtSetTypeConverter (
			XtRString,
			XtRFontSet,
			CvtStringToFontSet,
			(XtConvertArgList)screenConvertArg,
			1,
			XtCacheByDisplay,
			FreeFontSet
		);
		XtSetTypeConverter (
			XtRString,
			XtROlFont,
			CvtStringToOlFont,
			convertOlFontArgs,
			XtNumber(convertOlFontArgs),
			XtCacheNone,
			(XtDestructor)NULL
		);
		XtSetTypeConverter (
			XtRString,
			XtROlFontPrivate,
			CvtStringToOlFontPrivate,
			convertOlFontPrivateArgs,
			XtNumber(convertOlFontPrivateArgs),
			XtCacheNone,
			(XtDestructor)NULL
		);
		XtSetTypeConverter (
			XtRString,
			OlRChar,
			CvtStringToChar,
			(XtConvertArgList)NULL,
			(Cardinal)0,
			XtCacheNone,
			(XtDestructor)NULL
		);
		XtSetTypeConverter (
			XtRString,
			XtROlImPreeditStyle,
			CvtStringToOlImPreeditStyle,
			(XtConvertArgList)NULL,
			(Cardinal)0,
			XtCacheByDisplay,
			(XtDestructor)NULL
		);
		XtSetTypeConverter (
			XtRString,
			XtROlImStatusStyle,
			CvtStringToOlImStatusStyle,
			(XtConvertArgList)NULL,
			(Cardinal)0,
			XtCacheByDisplay,
			(XtDestructor)NULL
		);
		XtSetTypeConverter (
			XtRLocaleString,
			XtROlStr,
			CvtLocaleStringToOlStr,
			convertOlStrArgs,
			XtNumber(convertOlStrArgs),
			XtCacheNone | XtCacheRefCount,
			FreeOlStr
		);
		XtSetTypeConverter (
			XtRString,
			XtROlStr,
			CvtStringToOlStr,
			convertOlStrArgs,
			XtNumber(convertOlStrArgs),
			XtCacheNone | XtCacheRefCount,
			FreeOlStr
		);
		XtSetTypeConverter (
			XtRString,
			XtROlTextSource,
			CvtStringToOlTextSource,
			convertOlTextSourceArgs,
			XtNumber(convertOlTextSourceArgs),
			XtCacheNone,
		        (XtDestructor)NULL
		);
		XtSetTypeConverter (
			XtRString,
			XtRPixel,
			CvtStringToPixel,
			(XtConvertArgList)colorConvertArgs,
			2,
			XtCacheByDisplay,
			FreePixel
		);
		XtSetTypeConverter (
			XtRVisual,
			XtRColormap,
			CvtVisualToColormap,
			(XtConvertArgList)screenConvertArg,
			1,
			XtCacheByDisplay,
			(XtDestructor)0
		);
                XtSetTypeConverter(XtRString, 
				   XtROlEditMode, 
				   CvtStringToOlEditMode,
    				   (XtConvertArgList) NULL, 
				   (Cardinal) 0, 
				   XtCacheNone, 
				   (XtDestructor) NULL);

		XtSetTypeConverter(
			XtRString,
			XtROlScale,
		   	CvtStringToOlScale,
			(XtConvertArgList) NULL,
			(Cardinal) 0,
			XtCacheAll,
			(XtDestructor)NULL
		);

		(void)OlRegisterColorTupleListConverter();

		registered = True;
	}
	return;
} /* OlRegisterConverters() */

/**
 ** _OlAddOlDefineType()
 ** _OlRegisterOlDefineType() - Obsolete
 **/

void
_OlAddOlDefineType (String name, OlDefine value)
{
	(void)LookupOlDefine (name, value, ADD_TO_TABLE);
	return;
} /* _OlAddOlDefineType() */

void
_OlRegisterOlDefineType ( XtAppContext app_context,
			 String name, OlDefine value )
{
	(void)LookupOlDefine (name, value, ADD_TO_TABLE);
} /* END OF _OlRegisterOlDefineType() */

/**
 ** _OlCvtStringToGravity() - Obsolete
 **/

void
_OlCvtStringToGravity (XrmValue *args, Cardinal *num_args, XrmValue *from, XrmValue *to)
{

	CvtStringToGravity (
		toplevelDisplay,
		args,
		num_args,
		from,
		to,
		(XtPointer *)0
	);
	return;
} /* _OlCvtStringToGravity() */

/**
 ** _OlCvtStringToOlDefine() - Obsolete
 **/

void
_OlCvtStringToOlDefine (XrmValue *args, Cardinal *num_args, XrmValue *from, XrmValue *to)
{
	extern Display *	toplevelDisplay;

	CvtStringToOlDefine (
		toplevelDisplay,
		args,
		num_args,
		from,
		to,
		(XtPointer *)0
	);
	return;
} /* _OlCvtStringToOlDefine() */

/**
 ** CvtStringToDimension()
 **/

/*ARGSUSED5*/
static Boolean
CvtStringToDimension (Display *display, XrmValue *args, Cardinal *num_args, XrmValue *from, XrmValue *to, XtPointer *converter_data)
{
	DeclareConversionClass(display, "stringToDimension", (String)0);

	int			i;

	Screen *		screen;


	if (*num_args != 1)
		ConversionError (
			"wrongParameters",
			"String to Dimension conversion needs Screen* argument",
			(String *)0,
			(Cardinal *)0
		);
		/*NOTREACHED*/

	screen = *((Screen **)(args[0].addr));

	/*
	 * MORE: This will fail with huge input numbers.
	 */
	if (!IsScaledInteger((String)from->addr, &i, screen) || i < 0) {
		static String		_args[2]  = { 0 , 0 };
		Cardinal		_num_args = 1;

		_args[0] = (String)from->addr;
		ConversionWarning (
			"illegalString",
			"String to Dimension given illegal string: %s",
			_args,
			&_num_args
		);
		return (False);
	}

	ConversionDone (Dimension, (Dimension)i);
} /* CvtStringToDimension */

/**
 ** CvtStringToPosition()
 **/

/*ARGSUSED5*/
static Boolean
CvtStringToPosition (Display *display, XrmValue *args, Cardinal *num_args, XrmValue *from, XrmValue *to, XtPointer *converter_data)
{
	DeclareConversionClass(display, "stringToPosition", (String)0);

	int			i;

	Screen *		screen;


	if (*num_args != 1)
		ConversionError (
			"wrongParameters",
			"String to Position conversion needs Screen* argument",
			(String *)0,
			(Cardinal *)0
		);
		/*NOTREACHED*/

	screen = *((Screen **)(args[0].addr));

	if (!IsScaledInteger((String)from->addr, &i, screen)) {
		static String		_args[2]  = { 0 , 0 };
		Cardinal		_num_args = 1;

		_args[0] = (String)from->addr;
		ConversionWarning (
			"illegalString",
			"String to Position given illegal string: %s",
			_args,
			&_num_args
		);
		return (False);
	}

	ConversionDone (Position, (Position)i);
} /* CvtStringToPosition */

/**
 ** CvtStringToGravity()
 **/

/*ARGSUSED5*/
static Boolean
CvtStringToGravity (Display *display, XrmValue *args, Cardinal *num_args, XrmValue *from, XrmValue *to, XtPointer *converter_data)
{
	DeclareConversionClass(display, "stringToGravity", (String)0);

	int			i;

	NameValue *		p;


	/*
	 * This converter used to convert the list of gravities to
	 * quarks, then compare quarks instead of strings. Now that
	 * we use the XtCacheAll feature, quarkification becomes less
	 * important, as we will come here only once per gravity value
	 * (at most!)
	 */

	if (*num_args != 0)
		ConversionError (
			"wrongParameters",
			"String to Gravity conversion needs no arguments",
			(String *)0,
			(Cardinal *)0
		);
		/*NOTREACHED*/

	for (p = gravities; p->name; p++)
		if (CASELESS_STREQU((String)from->addr, p->name))
			break;

	if (!p->name) {
		static String		_args[2]  = { 0 , 0 };
		Cardinal		_num_args = 1;

		_args[0] = (String)from->addr;
		ConversionWarning (
			"illegalString",
			"String to Gravity given illegal string: %s",
			_args,
			&_num_args
		);
		return (False);
	}

	ConversionDone (int, p->value);
} /* CvtStringToGravity() */

/**
 ** CvtStringToCardinal()
 **/

/*ARGSUSED5*/
static Boolean
CvtStringToCardinal (Display *display, XrmValue *args, Cardinal *num_args, XrmValue *from, XrmValue *to, XtPointer *converter_data)
{
	DeclareConversionClass(display, "stringToCardinal", (String)0);

	int			i;


	if (*num_args != 0)
		ConversionError (
			"wrongParameters",
			"String to Cardinal conversion needs no arguments",
			(String *)0,
			(Cardinal *)0
		);
		/*NOTREACHED*/

	/*
	 * MORE: This will fail with huge input numbers.
	 */
	if (!IsInteger((String)from->addr, &i) || i < 0) {
		static String		_args[2]  = { 0 , 0 };
		Cardinal		_num_args = 1;

		_args[0] = (String)from->addr;
		ConversionWarning (
			"illegalString",
			"String to Cardinal given illegal string: %s",
			_args,
			&_num_args
		);
		return (False);
	}

	ConversionDone (Cardinal, (Cardinal)i);
} /* CvtStringToCardinal() */

/**
 ** CvtStringToOlDefine()
 **/

/*ARGSUSED5*/
static Boolean
CvtStringToOlDefine (Display *display, XrmValue *args, Cardinal *num_args, XrmValue *from, XrmValue *to, XtPointer *converter_data)
{
	DeclareConversionClass(display, "stringToOlDefine", (String)0);

	OlDefine		value;


	if (*num_args != 0)
		ConversionError (
			"wrongParameters",
			"String to OlDefine conversion needs no arguments",
			(String *)0,
			(Cardinal *)0
		);
		/*NOTREACHED*/


	value = LookupOlDefine((String)from->addr, (OlDefine)0, LOOKUP_VALUE);
	if (!value) {
		static String		_args[2]  = { 0 , 0 };
		Cardinal		_num_args = 1;

		_args[0] = (String)from->addr;
		ConversionWarning (
			"illegalString",
			"String to OlDefine given illegal string: %s",
			_args,
			&_num_args
		);
		return (False);
	}

	ConversionDone (OlDefine, value);
} /* CvtStringToOlDefine() */

/**
 ** CvtStringToModifiers()
 **/

static Boolean
CvtStringToModifiers (
	Display *		display,
	XrmValue *		args,
	Cardinal *		num_args,
	XrmValue *		from,
	XrmValue *		to,
	XtPointer *		converter_data
)
{
	DeclareConversionClass(display, "stringToModifiers", (String)0);

	static char		dummy_detail[]	= { LBRA , 'a', RBRA, 0 };
	static char		prefixed_key_buf[20];

	Modifiers		modifiers	= 0;


	if (*num_args)
		ConversionError (
			"wrongParameters",
			"String to Modifiers conversion needs no args",
			(String *)0,
			(Cardinal *)0
		);
		/*NOTREACHED*/

	if (from->addr) {
		XrmValue		_from;
		XrmValue		_to;

		String			prefixed_key;

		Cardinal		len;

		Boolean			success;

		OlKeyDef *		kd;


		len = Strlen((String)from->addr) + XtNumber(dummy_detail);
		if (len > XtNumber(prefixed_key_buf))
			prefixed_key = XtMalloc(len);
		else
			prefixed_key = prefixed_key_buf;

		strcpy (prefixed_key, (String)from->addr);
		strcat (prefixed_key, dummy_detail);

		_from.addr = (XtPointer)prefixed_key;
		_from.size = len;
		_to.addr   = 0;
		success = XtCallConverter(
			display,
			_OlStringToOlKeyDef,
			(XrmValuePtr)0,
			(Cardinal)0,
			&_from,
			&_to,
			(XtCacheRef *)0
		);

		if (prefixed_key != prefixed_key_buf)
			XtFree (prefixed_key);

		if (!success)
			return (False);

		kd = (OlKeyDef *)_to.addr;
		if (kd->used != 1) {
			static String		_args[2]  = { 0 , 0 };
			Cardinal		_num_args = 1;

			_args[0] = (String)from->addr;
			ConversionWarning (
				"illegalSyntax",
				"String to Modifiers found multiple keys: %s",
				_args,
				&_num_args
			);
			return (False);
		}

		modifiers = kd->modifier[0];

	} else {
		ConversionWarning (
			"nullString",
			"String to Modifiers given null string",
			(String *)0,
			(Cardinal *)0
		);
		return (False);
	}

	ConversionDone (Modifiers, modifiers);
} /* CvtStringToModifiers */

/**
 ** CvtStringToFont()
 ** FreeFont()
 **/

static Boolean
CvtStringToFont (Display *display, XrmValue *args, Cardinal *num_args, XrmValue *from, XrmValue *to, XtPointer *converter_data)
{
	DeclareConversionClass (display, "stringToFont", (String)0);

	Font			f;

	Screen *		screen;


	if (*num_args != 1)
		ConversionError (
			"wrongParameters",
			"String to Font conversion needs Screen* argument",
			(String *)0,
			(Cardinal *)0
		);
		/*NOTREACHED*/

	screen = *((Screen **)(args[0].addr));

	if (!IsFont("Font", screen, (String)from->addr, &f, converter_data))
		ConversionError (
			"noFont",
			"Unable to load any usable ISO 8859-1 font",
			(String *)0,
			(Cardinal *)0
		);

	ConversionDone (Font, f);
} /* CvtStringToFont */

static void
FreeFont (XtAppContext app, XrmValuePtr to, XtPointer closure, XrmValuePtr args, Cardinal *num_args)
{
	DeclareDestructionClass (app, "freeFont", (String)0);

	Screen *		screen;

#define free_the_font	(Boolean)closure


	if (*num_args != 1)
		DestructionError (
			"wrongParameters",
			"Free Font needs Screen* argument",
			(String *)0,
			(Cardinal *)0
		);
		/*NOTREACHED*/

	screen = *((Screen **)(args[0].addr));

	if (free_the_font)
		XUnloadFont (DisplayOfScreen(screen), *(Font*)to->addr);

#undef free_the_font
	return;
} /* FreeFont */

/**
 ** CvtStringToFontStruct()
 ** FreeFontStruct()
 **/

static Boolean
CvtStringToFontStruct (
	Display *		display,
	XrmValue *		args,
	Cardinal *		num_args,
	XrmValue *		from,
	XrmValue *		to,
	XtPointer *		converter_data
)
{
	DeclareConversionClass (display, "stringToFontStruct", (String)0);

	XFontStruct *		fs;

	Font			f;

	Screen *		screen;


	if (*num_args != 1)
		ConversionError (
			"wrongParameters",
			"String to FontStruct conversion needs Screen* argument",
			(String *)0,
			(Cardinal *)0
		);
		/*NOTREACHED*/

	screen = *((Screen **)(args[0].addr));

	if (!IsFont("FontStruct", screen, (String)from->addr, &f, converter_data))
		ConversionError (
			"noFont",
			"Unable to load any usable ISO 8859-1 font",
			(String *)0,
			(Cardinal *)0
		);

	/*
	 * Slight problem here: If the font ID returned is ``static''
	 * (as marked by the "converter_data"), then the following
	 * will never be freed. The problem is that Xlib doesn't
	 * provide a way to back out of the String -> Font -> FontStruct
	 * steps without trashing the Font as well as the FontStruct.
	 * But currently this will happen only once (Font can be static
	 * only for Xt default font), so not a big deal.
	 */
	if (!(fs = XQueryFont(display, f)))
		ConversionError (
			"badQueryFont",
			"XQueryFont failed on good font ID",
			(String *)0,
			(Cardinal *)0
		);

	ConversionDone (XFontStruct *, fs);
} /* CvtStringToFontStruct */

static void
FreeFontStruct (XtAppContext app, XrmValuePtr to, XtPointer closure, XrmValuePtr args, Cardinal *num_args)
{
	DeclareDestructionClass(app, "freeFontStruct", (String)0);

	Screen *		screen;

#define free_the_font	(Boolean)closure


	if (*num_args != 1)
		DestructionError (
			"wrongParameters",
			"Free FontStruct needs Screen* argument",
			(String *)0,
			(Cardinal *)0
		);
		/*NOTREACHED*/

	screen = *((Screen **)(args[0].addr));

	if (free_the_font)
		XFreeFont (DisplayOfScreen(screen), *(XFontStruct**)to->addr);

#undef free_the_font
	return;
} /* FreeFontStruct */

/**
 ** IsScaledInteger()
 **/

static Boolean
IsScaledInteger (String val, int *pint, Screen *screen)
{
	Boolean			ret;

	OlDefine		axis	= OL_HORIZONTAL;

	String			pri_val	= Strdup(val);
	String			rest	= 0;
	String			word;

	Fraction		d;

	Units			*pu;


	d = StrToFraction(pri_val, &rest);
	if (rest && *rest) {

		rest += strspn(rest, WORD_SEPS);
		while ((word = strtok(rest, WORD_SEPS))) {

			for (pu = units; pu->modifier; pu++)
				if (CASELESS_STREQU((char *)pu->modifier,word))
					break;
			if (!pu->modifier) {
				ret = FALSE;
				goto Return;
			}

			if (pu->axis)
				axis = pu->axis;
			else if (IsZeroFraction(pu->factor))
				goto Pixels;
			else
				d = MulFractions(d, pu->factor);
			rest = 0;

		}
		if (IsZeroFraction(d))
			*pint = 0;
		else {
			Fraction		mmtopixel;

			mmtopixel =
				(axis == OL_HORIZONTAL?
					NewFraction(
						WidthOfScreen(screen),
						WidthMMOfScreen(screen)
					)
				      : NewFraction(
						HeightOfScreen(screen),
						HeightMMOfScreen(screen)
					)
				);

			*pint = FractionToInt(MulFractions(d, mmtopixel));

			/*
			 * The value wasn't zero before the conversion
			 * to pixels, so don't let it be zero now. This
			 * ensures that all non-zero dimensions/positions
			 * are visible on the screen.
			 */
			if (*pint == 0)
				if (IsNegativeFraction(d))
					*pint = -1;
				else
					*pint = 1;
		}
		ret = TRUE;
	} else {
Pixels:		*pint = FractionToInt(d);
		ret = TRUE;
	}

Return:
	XtFree (pri_val);
	return (ret);
} /* IsScaledInteger() */

/**
 ** IsInteger()
 **/

static Boolean
IsInteger (String val, int *pint)
{
	String			rest	= 0;


	*pint = strtol(val, &rest, 0);
	return (!(rest && *rest));
} /* IsInteger() */

/**
 ** IsFont()
 **/

static Boolean
IsFont (
	String			who,
	Screen *		screen,
	String			str,
	Font *			p_font,
	XtPointer *		p_converter_data
)
{
	XrmRepresentation	type;
	XrmValue		value;

	Display *		display = DisplayOfScreen(screen);
	String			try;
	String                  defaultFont;

#define p_free_the_font (Boolean *)p_converter_data


	/*
	 * Assume we'll allocate the font.
	 */
	*p_free_the_font = True;

	/*
	 * If we were given a non-null font name other than
	 * the Xt default font OR the OL default bold font, try fetching it.
	 */
	if (str && *str && !CASELESS_STREQU(str, XtDefaultFont)
			&& !CASELESS_STREQU(str, (String)OlDefaultBoldFont)) {
		try = MapFont(screen, str, False, False, False);
		if (*p_font = LoadFont(display, try))
			return (True);
		XtDisplayStringConversionWarning (display, str, who);
	}

	/*
	 * Either we were asked to fetch the Xt default font or
	 * the font requested isn't known by the X server. Try
	 * the Xt default font.
	 */
	if (FetchResource(display, "xtDefaultFont", "XtDefaultFont", &type, &value)) {
		if (type == XrmPermStringToQuark(XtRString)) {
			*p_font = LoadFont(display, (char *)value.addr);
			if (*p_font)
				return (True);
			else {
				XtDisplayStringConversionWarning (
					display,
					(String)value.addr,
					"Font"
				);
			}

		} else if (type == XrmPermStringToQuark(XtRFont)) {
			*p_font = *(Font*)value.addr;
			*p_free_the_font = False;
			return (True);

		} else if (type == XrmPermStringToQuark(XtRFontStruct)) {
			*p_font = ((XFontStruct*)value.addr)->fid;
			*p_free_the_font = False;
			return (True);
		}
	}
	
	/*
	 * If a font wasn't specified and the Xt Default Font
	 * resource hasn't been set, use the Olit Default Font.
	 *
	 * The Olit default font has been "resource-ified" thru the
	 * application resource "XtNolDefaultFont". It is important
	 * that this resource be specified in XLFD format - for all
	 * the following processing to occur correctly. The default
	 * for this is a scalable 75x75 font - might want to change it
	 * if the hand tuned fonts are changed to the more appropriate
	 * 83x83. 
	 * If the resolution fields are "*" or "?" , we assume that the
	 * user wants resolution-independent fonts; we provide it based
	 * on the screen's H/V resolutions.  Else we retain the
	 * specified values. The Bold font is obtained by replacing the
	 * WEIGHT_NAME field with "Bold".
	 * We try to load the processed XLFD name . If this fails, we
	 * assume that the server does'nt support scalable fonts.
	 *
	 */
	defaultFont = _OlGetOlDefaultFont(screen);
	if (CASELESS_STREQU(str, (String)OlDefaultBoldFont)) {
		try = MapFont(screen, defaultFont, True, True, True);
		if ((*p_font = LoadFont(display, try)))
			return (True);

		try = MapFont(screen, defaultFont, False, True, True);

		if ((*p_font = LoadFont(display, try)))
			return (True);
	}
	try = MapFont(screen, defaultFont, True, True, False);
	if ((*p_font = LoadFont(display, try)))
		return (True);

	try = MapFont(screen, defaultFont, False, True, False);
	if ((*p_font = LoadFont(display, try)))
		return (True);

	/*
	 * Last resort is to find any reasonable font.
	 */
	*p_font = LoadFont(display, "-*-*-*-R-*-*-*-120-*-*-*-*-ISO8859-1");
	if (*p_font)
		return (True);

	return (False);

#undef p_free_the_font
#undef display
} /* IsFont */


/*
 * MapFont()
 */
static String
MapFont(Screen* screen, String str, Boolean resFlag, Boolean psFlag,
	Boolean boldFlag)
{
String start = str;
String end = str;
static char	new[_XLFD_MAX_LENGTH + 1];
size_t len =0;

	/* extract the first XLFD name from the string of font
		names */

	for(; *end != '\0' && *end != ','; end++)
			;
	/* backup to remove trailing spaces ; 
	 * space at the end is not allowed in XLoadFont */
	 while(isspace(*(--end)))
		;	

	len = (size_t)(end - start + 1);

	if(len > _XLFD_MAX_LENGTH)
		return(str);

	strncpy(new,(const char *)str,len);
	new[len] = '\0';

	return(ParseXLFDName(screen, new, resFlag, psFlag, boldFlag));

} /* MapFont */

/**
 ** LoadFont()
 **/

static Font
LoadFont (
	Display *		display,
	String			name
)
{
	Font			f	= 0;

#if	defined(THIS_WORKS)
	char **			list;

	int			nfonts	= 0;


	/*
	 * We avoid a protocol error by looking first before loading.
	 */
	list = XListFonts(display, name, 1, &nfonts);
	if (nfonts && list)
		f = XLoadFont(display, list[0]);
	if (list)
		XFreeFontNames (list);
#else
	PrevErrorHandler = XSetErrorHandler(IgnoreBadName);

	GotBadName = False;

	f = XLoadFont(display, name);
	XFlush (display);
	XSync (display, False);

	if (GotBadName)
		f = 0;

	(void)XSetErrorHandler (PrevErrorHandler);
#endif

	return (f);
} /* LoadFont */

/**
 ** IgnoreBadName()
 **/

static int
IgnoreBadName (Display *display, XErrorEvent *event)
{
	if (event->error_code == BadName) {
		GotBadName = True;
		return (0);
	} else
		return ((*PrevErrorHandler)(display, event));
}

/**
 ** LookupOlDefine()
 **/

#define BUF_SIZE		512	/* number of characters	*/

#define LOWER_CASE(S,D) \
    {									\
	register char *	source	= (S);					\
	register char *	dest	= (D);					\
	register unsigned int	uich;					\
									\
	for (; (uich = *source) != 0; source++, dest++) {		\
		LOWER_IT (uich);					\
		*dest = (char)uich;					\
	}								\
	*dest = 0;							\
    }

static OlDefine
LookupOlDefine (String name, OlDefine value, int flag)
{
	char		buf[BUF_SIZE];		/* local buffer		*/
	char *		c_name;			/* lower-cased name	*/
	Cardinal	length = (Cardinal) strlen(name);
	OlDefineNode *	self;
	Bucket *	bucket;
	XrmQuark	quark;

	c_name = (char *) (length < (Cardinal)BUF_SIZE ? buf :
			XtMalloc(length * sizeof(char)));

	LOWER_CASE(name, c_name)

	quark = XrmStringToQuark( (flag == ADD_TO_TABLE &&
				c_name[0] == 'o' &&
				c_name[1] == 'l' &&
				c_name[2] == '_' ? (c_name + 3) : c_name));

	if (c_name != buf) {			/* Free the buffer	*/
		XtFree(c_name);
	}

	bucket = &define_table[HASH_QUARK(quark)];

	self = bucket->array;

	if (flag == ADD_TO_TABLE || self != (OlDefineNode *) NULL) {
		Cardinal i;
					/* Is the quark in the array ?	*/

		for (i=0; i < bucket->elements; ++i) {
			if (self[i].quark == quark) {
				return(self[i].value);
			}
		}

				/* If we're here, we didn't find the
				 * value in the table.			*/

		if (flag == ADD_TO_TABLE) {
			++bucket->elements;

			bucket->array = (OlDefineNode *)
				XtRealloc((char *)bucket->array,
				(bucket->elements * sizeof(OlDefineNode)));
			
			self	= &bucket->array[bucket->elements - 1];
			self->quark = quark;
			self->value = value;
			return(self->value);
		}
	}
	return(0);

} /* LookupOlDefine() */

/**
 ** NewFraction()
 ** StrToFraction()
 ** MulFractions()
 ** FractionToInt()
 **/

static Fraction
NewFraction (int numerator, int denominator)
{
	Fraction		ret;

	ret.numerator   = (long)numerator;
	ret.denominator = (long)denominator;
	return (ret);
}

static Fraction
StrToFraction (String str, String *p_rest)
{
	Fraction		ret;

	Boolean			negative	= False;
	Boolean			in_fraction	= False;


	ret.numerator   = 0;
	ret.denominator = 1;

	/*
	 * Just in case we don't have a number here....
	 */
	if (p_rest)
		*p_rest = str;

	while (isspace(*str))
		str++;

	switch (*str) {
	case MINUS:
		negative = True;
		/*FALLTHROUGH*/
	case PLUS:
		str++;
	}

	if (!isdigit(*str) && *str != DECPOINT)
		return (ret);

	do {
		if (*str == DECPOINT)
			in_fraction = True;
		else {
			ret.numerator = ret.numerator * 10L + DIGIT(*str);
			if (in_fraction)
				ret.denominator *= 10L;
		}
	} while (isdigit(*++str) || *str == DECPOINT);

	if (p_rest)
		*p_rest = str;

	if (negative)
		ret.numerator = -ret.numerator;

	return (ret);
}

static Fraction
MulFractions (Fraction a, Fraction b)
{
	Boolean			negative = False;


	a.numerator *= b.numerator;
	a.denominator *= b.denominator;

	/*
	 * Save some grief with "long" overflow. If the denominator
	 * is really big, reduce both numerator and denominator to
	 * bring the denominator down to a reasonable size. Essentially
	 * what we're doing here is throwing away the least significant
	 * bits, leaving enough approximately 4 significant DIGITS.
	 * We check the denominator, because we don't want to reduce
	 * it to zero.
	 *
	 * ASSUMPTION: We're not chaining many calculations with these
	 * Fractions, so we don't worry about accumulated error.
	 */
	if (a.numerator < 0) {
		negative = True;
		a.numerator = -a.numerator;
	}
	while (a.denominator > 8192) {
		a.numerator   >>= 1;
		a.denominator >>= 1;
	}
	if (negative)
		a.numerator = -a.numerator;

	return (a);
}

static int
FractionToInt (Fraction a)
{
	long			half = a.denominator / 2;

	if (a.denominator < 1)
		return (0);	/* undefined */
	if (a.numerator < half)
		return (0);
	else
		return ((a.numerator + half) / a.denominator);
}

/**
 ** FetchResource()
 **/

static Boolean
FetchResource (Display *display, String name, String c_class, XrmRepresentation *p_rep_type, XrmValue *p_value)
{
	XrmName			xrm_name[2];
	XrmClass		xrm_class[2];


	xrm_name[0]  = XrmStringToName(name);
	xrm_name[1]  = 0;
	xrm_class[0] = XrmStringToClass(c_class);
	xrm_class[1] = 0;
	return (XrmQGetResource(
		XtDatabase(display),
		xrm_name,
		xrm_class,
		p_rep_type,
		p_value
	));
}

/**
 ** caseless_strcmp()
 ** caseless_strncmp()
 **/

static int
caseless_strcmp (String s1, String s2)
{
	if (s1 == s2)
		return (0);
	return (CompareISOLatin1(s1, s2, 0));
} /* caseless_strcmp() */

static int
caseless_strncmp (String s1, String s2, register int n)
{
	if (s1 == s2)
		return (0);
	return (CompareISOLatin1(s1, s2, n));
} /* caseless_strncmp() */

/**
 ** CompareISOLatin1()
 **/

/*
 * Stolen shamelessly (but why should it have been necessary?)
 * from Xt/Converters.c. Reformatted to fit the local style,
 * to move the good stuff into the macro LOWER_IT, and added
 * the "count" argument.
 */

static int
CompareISOLatin1 (String first, String second, register int count)
{
	register unsigned char *ap	   = (unsigned char *)first;
	register unsigned char *bp	   = (unsigned char *)second;

	register Boolean	dont_count = (count == 0);


	for (; (dont_count || --count >= 0) && *ap && *bp; ap++, bp++) {
		register unsigned char	a  = *ap;
		register unsigned char	b  = *bp;

		if (a != b) {
			LOWER_IT (a);
			LOWER_IT (b);
			if (a != b)
				break;
		}
	}
	return ((int)*bp - (int)*ap);
}


static Boolean
CvtStringToPixel (Display *display, XrmValue *args, Cardinal *num_args, XrmValue *from, XrmValue *to, XtPointer *closure_ret)
{
	DeclareConversionClass(display, "stringToPixel", (String)0);

	String		str = (String)from->addr;
	XColor		screenColor;
	XColor		exactColor;
	Screen		*screen;
	XtPerDisplay	pd = _XtGetPerDisplay(display);
	Colormap	colormap;
	Status		status;
	char 		*blackpixel = "black",
			*whitepixel = "white";

	if (*num_args != 2)
		ConversionError (
			"wrongParameters",
			"String to pixel conversion needs screen and colormap arguments",
			(String *)0,
			(Cardinal *)0
			);

	screen = *((Screen **) args[0].addr);
	colormap = *((Colormap *) args[1].addr);

	if (CompareISOLatin1(str, XtDefaultBackground, 0) == 0) {
		if (pd->rv)
			str = blackpixel;
		else
			str = whitepixel;
	}
	if (CompareISOLatin1(str, XtDefaultForeground, 0) == 0) {
		if (pd->rv)
			str = whitepixel;
		else
			str = blackpixel;
	}

	if (*str == '#') {  /* some color rgb definition */

		status = XParseColor(DisplayOfScreen(screen), 
			colormap, (char*)str, &screenColor);

		if (status == 0) {
			static String _args[2]  = { 0 , 0 };
			Cardinal _num_args = 1;

			_args[0] = str;
			ConversionWarning ("badFormat",
				"RGB color specification \"%s\" has invalid format",
				_args, &_num_args);
			*closure_ret = False;
			return False;
		} else
			status = XAllocColor(
				DisplayOfScreen(screen),
				colormap, &screenColor);
	} else  /* some color name */

		status = XAllocNamedColor(DisplayOfScreen(screen),
			colormap, (char*)str, &screenColor, &exactColor);
	if (status == 0) {
		String		msg, type;
		static String	_args[2]  = { 0 , 0 };
		Cardinal	_num_args = 1;

		_args[0] = str;
		/* Server returns a specific error code but Xlib discards it.  Ugh */
		if (*str == '#' || XLookupColor(
			DisplayOfScreen(screen), colormap,
			(char*)str, &exactColor,
			&screenColor)) {
			type = "noColormap";
			msg = "Cannot allocate colormap entry for \"%s\"";
		} else {
			type = "badValue";
			msg = "Color name \"%s\" is not defined in server database";
		}

		ConversionWarning (type, msg, _args, &_num_args);
		*closure_ret = False;
		return False;
	} else {
		*closure_ret = (char*)True;
		ConversionDone (Pixel, screenColor.pixel);
	}
}

static void
FreePixel (XtAppContext app, XrmValuePtr to, XtPointer closure, XrmValuePtr args, Cardinal *num_args)
{
	DeclareDestructionClass (app, "freePixel", (String)0);

	Screen		*screen;
	Colormap	colormap;

	if (*num_args != 2)
		DestructionError (
			"wrongParameters",
			"Freeing a pixel requires screen and colormap arguments",
			(String *)0,
			(Cardinal *)0
			);

	screen = *((Screen **) args[0].addr);
	colormap = *((Colormap *) args[1].addr);

	if (closure) {
		XFreeColors( DisplayOfScreen(screen), colormap,
			(unsigned long*)to->addr, 1, (unsigned long)0);
	}
}

/*
 * Obtain a colormap for a given visual structure.
 * Since the converter does caching colormaps are
 * shared within an application.
 * 
 */
static Boolean
CvtVisualToColormap (Display *display, XrmValue *args, Cardinal *num_args, XrmValue *from, XrmValue *to, XtPointer *closure_ret)
{
	DeclareConversionClass(display, "visualToColormap", (String)0);

	Visual		*visual = *((Visual **)from->addr);
	Screen		*screen;
	Window		rootwin;
	Colormap	colormap;
	long		vinfo_mask;
	XVisualInfo	vinfo_template, *retVal;
	int		num;
	


	if (*num_args != 1)
		ConversionError (
			"wrongParameters",
			"Visual to colormap conversion needs screen arguments",
			(String *)0,
			(Cardinal *)0
			);

	screen = *((Screen **) args[0].addr);
	rootwin = RootWindowOfScreen(screen);

	/*
	 * Special case where Visual is the 
	 * default visual of screen.
	 */
	if (visual == DefaultVisualOfScreen (screen)) {
		colormap = DefaultColormapOfScreen (screen);
#if	OLIT_DEBUG
		printf("CvtVisualToColormap: sharing default colormap %X\n", colormap);
#endif	/* OLIT_DEBUG */
		ConversionDone (Colormap, colormap);
	}

	/*
	 * validate visual
	 */
	vinfo_template.visual = visual;
	vinfo_mask = VisualNoMask;
	if ((retVal = XGetVisualInfo(display, vinfo_mask,
		&vinfo_template, &num)) == NULL) {
		ConversionWarning ("illegalVisual",
			"Visual to Colormap given illegal visual",
			(String *)0, (Cardinal *)0);
		return False;
	}
	XtFree((char *)retVal);

	/*
	 * If it's a dynamic visual look for a 
	 * pre-allocated colormap to share.
	 */
	#ifdef	__cplusplus
	if (visual->c_class % 2) {
	#else
	if (visual->class %2) {
	#endif	/* __cplusplus */
		Atom		actual_type;
		int		actual_format;
		unsigned long	nitems, bytes_after;
		XStandardColormap *scm;

		/* get the property's size */
		XGetWindowProperty(display, rootwin,
			XA_RGB_DEFAULT_MAP, 0L, 1L, False,
			AnyPropertyType, &actual_type,
			&actual_format, &nitems, &bytes_after,
			(unsigned char **)&scm);

		/* get the entire property */
		XGetWindowProperty(display, rootwin,
			XA_RGB_DEFAULT_MAP, 0L,
			bytes_after/4 + 1, False,
			AnyPropertyType, &actual_type,
			&actual_format, &nitems, &bytes_after,
			(unsigned char **)&scm);

		/* search for match */
		nitems /= (sizeof(XStandardColormap)/4);
		for (; nitems > 0; ++scm, --nitems)
			if (scm->visualid == visual->visualid) {
				#ifdef	DEBUG
				printf("CvtVisualToColormap: sharing dynamic"
					" colormap %X\n", scm->colormap);
				#endif	/* DEBUG */
				ConversionDone(Colormap,
					scm->colormap);
			}
	}

	/* allocate new colormap */
	colormap = XCreateColormap(display, rootwin, visual,
		AllocNone);

	#ifdef	DEBUG
	#	ifdef	__cplusplus
	if (visual->c_class % 2)
	#	else
	if (visual->class %2)
	#	endif	/* __cplusplus */
		printf("CvtVisualToColormap: creating new dynamic colormap"
			" %X\n", colormap);
	else
		printf("CvtVisualToColormap: sharing new static colormap %X\n",
			colormap);
	#endif	/* DEBUG */

	ConversionDone(Colormap, colormap);
}

/* 
 * CvtLocaleStringToOlStrPrivate: This function is to be ONLY accessed through 
 * XtCallConverter from converters in this file. 
 * Do not invoke this converter directly from an application. Unfortunately,
 * there is no fool-proof way to prevent that. Therefore, it is very important
 * that the type XtROlStrPrivate is not visible outside this file. 
 * The whole idea of having this converter is that the VendorShellWidget 
 * does not have a text format in its widget rec; in fact it has it in its 
 * extension record. This converter allows us  an extra level of 
 * indirection. It allows us to to set up the extra args
 * passed to this converter correctly. If the widget asociated with 
 * the conversion attempt is an Xt defined  subclass of VendorShell, we pass 
 * vendorOlStrPrivateArgs as extra args to this converter, else
 * we pass convertOlStrPrivateArgs to this connverter. 
 *
 * Also see CvtLocaleStringToOlStr...  
 */

static Boolean
CvtLocaleStringToOlStr(Display *display, 
		 XrmValue *args, 
		 Cardinal *num_args, 
		 XrmValue *from,
		 XrmValue *to,
		 XtPointer *converter_data)
{
        DeclareConversionClass (display, "localeStringToOlStr", (String)0);
	OlStrRep		text_format;
	static wchar_t 		*ws;
	static char		*string;
	int			len;
	Boolean 		success = True;
	XrmValue		src = {0, 0};
	Cardinal		numArgs = 1;
	Cardinal		sizeofOlStrRep = sizeof(OlStrRep);


        if (*num_args != 1)
                ConversionError (
                     "wrongParameters",
                     "LocaleString to OlStr conversion needs text format",
                     (String *)NULL,
                     (Cardinal *)NULL); /* NOT REACHED */

	text_format = *((OlStrRep *)(args[0].addr)); 
	if(text_format != OL_SB_STR_REP)
		src.addr = (XtPointer)XtNewString(dgettext(OlMsgsDomain, 
							(char *)from->addr));
	else
		src.addr = XtNewString((char *)from->addr);
	src.size = strlen((char *)src.addr) + 1;
	success = CvtStringToOlStrPrivate(display, (XrmValuePtr)&args[0],
		 &numArgs, &src, to, converter_data);
	if((char *)src.addr)
		XtFree((char *)src.addr);

return(success);
					
} /* CvtLocaleStringToOlStr */

static Boolean ResourceQuarkToOffset(widget_class, name, offset)
    WidgetClass widget_class;
    XrmName     name;
    Cardinal    *offset;
{
    register WidgetClass     wc;
    register Cardinal        i;
    register XrmResourceList res, *resources;
 
    for (wc = widget_class; wc; wc = wc->core_class.superclass) {
        resources = (XrmResourceList*) wc->core_class.resources;
        for (i = 0; i < wc->core_class.num_resources; i++, resources++) {
            res = *resources;
            if (res->xrm_name == name) {
                *offset = -res->xrm_offset - 1;
                return True;
            }
        } /* for i in resources */
    } /* for wc in widget classes */
    (*offset) = 0;
    return False;
}

/*
 *************************************************************************
 * _OlGetTextFormat - Retrieves the text_format for widget
 ****************************procedure*header*****************************
 */
static void
_OlGetTextFormat(Widget w, Cardinal *size, XrmValue *ret_val)
{
Cardinal	num_params = 1;
Cardinal	offset = 0;
WidgetClass	class;

	class = XtClass(w);
	*size = sizeof(OlStrRep);

	/* This is deliberate: we need ONLY these classes ;
		not their subclasses */
	if (class == transientShellWidgetClass ||
		class == topLevelShellWidgetClass ||
		class == applicationShellWidgetClass)
		_OlGetVendorTextFormat(w,size,ret_val);

	else  {
		if (!ResourceQuarkToOffset(w->core.widget_class,
		    XrmStringToQuark((String)XtNtextFormat), &offset))
		    XtAppWarningMsg(XtWidgetToApplicationContext(w),
		       "invalidResourceName","computeArgs",XtCXtToolkitError,
		       "Cannot find resource name %s as argument to conversion",
		       (String)XtNtextFormat,&num_params);
		ret_val->addr = (XtPointer) ((char *)w + offset);
	}
 
}

/* 
 * CvtStringToOlStr 
 */

static Boolean
CvtStringToOlStr(Display *display, 
		 XrmValue *args, 
		 Cardinal *num_args, 
		 XrmValue *from,
		 XrmValue *to,
		 XtPointer *converter_data)
{
        DeclareConversionClass (display, "stringToOlStr", (String)0);
	Widget			w;
	Boolean			success;
	XrmValue		xrmValue;
	Cardinal		numArgs = 1;
	Cardinal		sizeofOlStrRep = sizeof(OlStrRep);

        if (*num_args != 1)
                ConversionError (
                     "wrongParameters",
                     "String to OlStr conversion needs text format",
                     (String *)NULL,
                     (Cardinal *)NULL); /* NOT REACHED */

	success = CvtStringToOlStrPrivate(display, (XrmValuePtr)&args[0],
		&numArgs, from, to, converter_data);

return(success);
					
} /* CvtStringToOlStr */

/* 
 * CvtStringToOlStrPrivate: This function is to be ONLY accessed through 
 * XtCallConverter from converters in this file. 
 * Do not invoke this converter directly from an application. Unfortunately,
 * there is no fool-proof way to prevent that. Therefore, it is very important
 * that the type XtROlStrPrivate is not visible outside this file. 
 * The whole idea of having this converter is that the VendorShellWidget 
 * does not have a text format in its widget rec; in fact it has it in its 
 * extension record. This converter allows us  an extra level of 
 * indirection. It allows us to to set up the extra args
 * passed to this converter correctly. If the widget asociated with 
 * the conversion attempt is an Xt defined  subclass of VendorShell, we pass 
 * vendorOlStrPrivateArgs as extra args to this converter, else
 * we pass convertOlStrPrivateArgs to this connverter. 
 *
 * Also see CvtStringToOlStr...  
 */

static Boolean
CvtStringToOlStrPrivate(Display *display, 
		 XrmValue *args, 
		 Cardinal *num_args, 
		 XrmValue *from,
		 XrmValue *to,
		 XtPointer *converter_data)
{
        DeclareConversionClass (display, "stringToOlStrPrivate", (String)0);
	OlStrRep		text_format;
	static wchar_t 		*ws;
	static char		*string;
	int			len;
	Boolean 		success = True;


        if (*num_args != 1)
                ConversionError (
                     "wrongParameters",
                     "String to OlStrPrivate conversion needs text format",
                     (String *)NULL,
                     (Cardinal *)NULL); /* NOT REACHED */

	text_format = *((OlStrRep *)(args[0].addr)); 
	*converter_data = (XtPointer)False;
	switch(text_format) {
		case OL_SB_STR_REP:
		case OL_MB_STR_REP:
			if(to->addr != NULL && to->size < sizeof(OlStr)) {
			to->size = sizeof(OlStr);
			success = False;
			} else {
			
				string = (char *)XtMalloc((Cardinal)
					(strlen((char *)from->addr)+1));
				strcpy((char *)string,(char *)from->addr);

				if(to->addr == NULL)  
					to->addr = (XtPointer)&string; 
				else  
					*((OlStr *)to->addr) = (OlStr)string; 
				
				to->size = sizeof(OlStr);
				*converter_data = (XtPointer)True;
				
				success = True;
			}
			break;	
		case OL_WC_STR_REP:
			if(to->addr != NULL && to->size < sizeof(OlStr)) {
			to->size = sizeof(OlStr);
			success = False;
			} else {

				len = strlen((char *)from->addr) + 1;
				ws = (wchar_t *)XtMalloc(sizeof(wchar_t)*len);
				(void)mbstowcs(ws, (char *)from->addr, len);

				if(to->addr == NULL)  
					to->addr = (XtPointer)&ws; 
				else
					*((OlStr *)to->addr) = (OlStr)ws;

				to->size = sizeof(OlStr);
				*converter_data = (XtPointer)True;
				
				success = True;
			}
			break; 
		default:
			success = False;
			XtDisplayStringConversionWarning (display, 
							from->addr, 
							XtROlStr);
			OlError(dgettext(OlMsgsDomain,
"Unrecognizable text format passed to \
String to OlStr Converter: text format resource occurs after string/label \
resource in the resource list of passed widget?\n"));
			break;
			
	} /* switch */
if(success == FALSE)
	XtDisplayStringConversionWarning(display, from->addr, XtROlStr);

return(success);
					
} /* CvtStringToOlStrPrivate */

static void
FreeOlStr (
	XtAppContext	app,
	XrmValuePtr	to,
	XtPointer	closure,
	XrmValuePtr	args,
	Cardinal *	num_args)
{
	DeclareDestructionClass(app, "freeOlStr", (String)0);
	char *string = ((Boolean)closure && to && to->addr) ?
		*((char **)(to->addr)) : NULL;
	if (string)
		XtFree(string);
	return;
} /* FreeOlStr */


/*
 * OlTextSourceConverter
 *
 */

static Boolean
CvtStringToOlTextSource(Display *	display,
			XrmValue *	args,
			Cardinal *	num_args,
			XrmValue *	from,
			XrmValue *	to,
			XtPointer *	converter_data)
{
    DeclareConversionClass (display, "stringToOlTextSource", (String)0);
    static OlSourceType	source_type;
    static OlStrRep	text_format;
    static OlTextSource text_source;
    Boolean 		success;
    Cardinal		numArgs = 1;

    if (*num_args != 2)
	ConversionError (
			 "wrongParameters",
			 "String to OlTextSourceType conversion needs two args",
			 (String *)NULL,
			 (Cardinal *)NULL); /* NOT REACHED */

    if (to->addr != NULL) {
	if (to->size < sizeof(OlTextSource)) {
	    to->size = sizeof(OlTextSource);
	    return(False);
	} else if (to->size > sizeof(OlTextSource)) {
	    to->size = sizeof(OlTextSource);
	}
    } else {
	to->addr = (XtPointer) &text_source;
	to->size = sizeof(OlTextSource);
    }
    
    source_type = *((OlSourceType *)(args[0].addr)); 
    text_format = *((OlStrRep *)(args[1].addr)); 
    switch(source_type) {
    case OL_STRING_SOURCE:
	success = CvtStringToOlStrPrivate(display, (XrmValuePtr)&args[1],
		&numArgs, from, to, converter_data);
	break;
    case OL_DISK_SOURCE:
	*((String *) to->addr) = (String) from->addr;
	success = TRUE;
	break;
    case OL_TEXT_BUFFER_SOURCE:
    case OL_OLTEXT_BUFFER_SOURCE:
	success = False;
	XtDisplayStringConversionWarning (display, 
					  from->addr, 
					  XtROlTextSource);
	OlError(dgettext(OlMsgsDomain, " XtNsourceType illegally set either \
to OL_TEXT_BUFFER_SOURCE or to OL_OLTEXT_BUFFER_SOURCE. Conversion of XtNsource \
resource from String to a textbuffer is not permitted. Please use either \
OL_STRING_SOURCE or OL_DISK_SOURCE.\n"));
	break;
    default:
	success = False;
	XtDisplayStringConversionWarning (display, 
					  from->addr, 
					  XtROlTextSource);
	OlError(dgettext(OlMsgsDomain, "Unrecognizable source type passed to \
String to OlTextSource Converter: XtNsourceType resource occurs after XtNsource \
in the resource list of passed widget?\n"));
	break;
			
    }
    return(success);
					
}

/**
 ** CvtStringToOlImPreeditStyle()
 **/

static Boolean
CvtStringToOlImPreeditStyle(Display *display, 
		 XrmValue *args, 
		 Cardinal *num_args, 
		 XrmValue *from,
		 XrmValue *to,
		 XtPointer *converter_data)
{
        DeclareConversionClass (display, "stringToOlImPreeditStyle", (String)0);
	Boolean 		success = True;
	static Boolean		haveQuarks = False;
	static XrmQuark	q, QonTheSpot, QoverTheSpot, QrootWindow, Qnone;
	static OlImPreeditStyle retval = OL_NO_PREEDIT;


        if (*num_args != 0)
                ConversionError (
                     "wrongParameters",
                     "String to OlImPreeditStyle conversion needs no args",
                     (String *)NULL,
                     (Cardinal *)NULL); /* NOT REACHED */

	if(haveQuarks == False) {	
		QonTheSpot = XrmStringToQuark("onTheSpot");
		QoverTheSpot = XrmStringToQuark("overTheSpot");
		QrootWindow = XrmStringToQuark("rootWindow");
		Qnone   = XrmStringToQuark("none");
		haveQuarks = True;
	}

	q = XrmStringToQuark(from->addr);
	if(q == QonTheSpot) 
		retval = OL_ON_THE_SPOT;
	else if(q == QoverTheSpot)
		retval = OL_OVER_THE_SPOT;
	else if(q == QrootWindow)
		retval = OL_ROOT_WINDOW;
	else if(q == Qnone)
		retval = OL_NO_PREEDIT;
	else
		success = False;
 	
	if(success == True) {

		if(to->addr != NULL && to->size < 
					sizeof(OlImPreeditStyle)) {
		to->size = sizeof(OlImPreeditStyle);
		success = False;
		} else
			if(to->addr == NULL) {
				to->addr = (XtPointer)&retval;
				to->size = sizeof(OlImPreeditStyle);
				success = True;
			} else {
				*((OlImPreeditStyle *)to->addr) = retval;
				to->size = sizeof(OlImPreeditStyle);
				success = True;
			}

	} else
		XtDisplayStringConversionWarning (display, from->addr, 
							XtROlImPreeditStyle);
		
				 
return(success);
					
} /* CvtStringToOlImPreeditStyle */


/**
 ** CvtStringToOlImStatusStyle()
 **/

static Boolean
CvtStringToOlImStatusStyle(Display *display, 
		 XrmValue *args, 
		 Cardinal *num_args, 
		 XrmValue *from,
		 XrmValue *to,
		 XtPointer *converter_data)
{
        DeclareConversionClass (display, "stringToOlImStatusStyle", (String)0);
	Boolean 		success = True;
	static Boolean		haveQuarks = False;
	static XrmQuark	q, Qfooter, QrootWindow, Qnone;
	static OlImStatusStyle retval = OL_NO_STATUS;


        if (*num_args != 0)
                ConversionError (
                     "wrongParameters",
                     "String to OlImStatusStyle conversion needs no args",
                     (String *)NULL,
                     (Cardinal *)NULL); /* NOT REACHED */

	if(haveQuarks == False) {	
		Qfooter = XrmStringToQuark("imDisplaysInClient");
		QrootWindow = XrmStringToQuark("imDisplaysInRoot");
		Qnone = XrmStringToQuark("none");
		haveQuarks = True;
	}

	q = XrmStringToQuark(from->addr);
	if(q == Qfooter) 
		retval = OL_IM_DISPLAYS_IN_CLIENT;
	else if(q == QrootWindow)
		retval = OL_IM_DISPLAYS_IN_ROOT;
	else if(q == Qnone)
		retval = OL_NO_STATUS;
	else
		success = False;
 	
	if(success == True) {

		if(to->addr != NULL && to->size < 
					sizeof(OlImStatusStyle)) {
		to->size = sizeof(OlImStatusStyle);
		success = False;
		} else
			if(to->addr == NULL) {
				to->addr = (XtPointer)&retval;
				to->size = sizeof(OlImStatusStyle);
				success = True;
			} else {
				*((OlImStatusStyle *)to->addr) = retval;
				to->size = sizeof(OlImStatusStyle);
				success = True;
			}

	} else
		XtDisplayStringConversionWarning (display, from->addr, 
							XtROlImStatusStyle);
		
				 
return(success);
					
} /* CvtStringToOlImStatusStyle */

/*
 *************************************************************************
 * CvtStringToChar - this routine converts a string to a mnemonic
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
CvtStringToChar(Display *dpy, 
		  XrmValue *args, 
		  Cardinal *num_args, 
		  XrmValue *from_val, 
		  XrmValue *to_val, 
		  XtPointer *converter_data)
{
	static unsigned char	mnemonic;


	/* 
         * For now we require the mnemonics to be no more than
	 * one byte long, in any locale.
 	 */

	if(mblen((char *)from_val->addr,MB_CUR_MAX) == 1)
		mnemonic = *((unsigned char *) from_val->addr);
	else
		mnemonic = '\0'; 
	if ((unsigned char *)(to_val->addr) == (unsigned char *)NULL ||
			to_val->size != sizeof(char)) {
		to_val->addr = (XtPointer) &mnemonic;
		to_val->size = sizeof(char);
	} else
		*((unsigned char *)(to_val->addr)) = mnemonic;

	return (True);
} /* END OF CvtStringToChar() */

/* 
 * CvtStringToOlFont 
 */

static Boolean
CvtStringToOlFont(Display *display, 
		 XrmValue *args, 
		 Cardinal *num_args, 
		 XrmValue *from,
		 XrmValue *to,
		 XtPointer *converter_data)
{
        DeclareConversionClass (display, "stringToOlFont", (String)0);
	Widget			w;
	Boolean			success;
	WidgetClass		class;

        if (*num_args != 1)
                ConversionError (
                     "wrongParameters",
                     "String to OlFont conversion needs Widget",
                     (String *)NULL,
                     (Cardinal *)NULL); /* NOT REACHED */

	w = *((Widget *)(args[0].addr));
	class = XtClass(w);

	/* This is deliberate: we need ONLY these classes ;
		not their subclasses */
	if( class == transientShellWidgetClass ||
		class == topLevelShellWidgetClass ||
		class == applicationShellWidgetClass) {
		OlVendorSetConverters(TRUE);
		success = XtConvertAndStore(w, XtRString, from, XtROlFontPrivate, to);
		OlVendorSetConverters(FALSE);
	} else
		success = XtConvertAndStore(w, XtRString, from, XtROlFontPrivate, to);

return(success);
					
} /* CvtStringToOlFont */

/* 
 * CvtStringToOlFontPrivate: This function is to be ONLY accessed through 
 * XtCallConverter from converters in this file. 
 * Do not invoke this converter directly from an application. Unfortunately,
 * there is no fool-proof way to prevent that. Therefore, it is very important
 * that the type XtROlFontPrivate is not visible outside this file. 
 * The whole idea of having this converter is that the VendorShellWidget 
 * does not have a text format in its widget rec; in fact it has it in its 
 * extension record. This converter allows us  an extra level of 
 * indirection. It allows us to to set up the extra args
 * passed to this converter correctly. If the widget asociated with 
 * the conversion attempt is an Xt defined  subclass of VendorShell, we pass 
 * vendorOlFontPrivateArgs as extra args to this converter, else
 * we pass convertOlFontPrivateArgs to this connverter. 
 *
 * Also see CvtStringToOlFont...  
 */

/**
 ** CvtStringToFontPrivate()
 **/

static Boolean
CvtStringToOlFontPrivate(Display *display, 
		 XrmValue *args, 
		 Cardinal *num_args, 
		 XrmValue *from,
		 XrmValue *to,
		 XtPointer *converter_data)
{
        DeclareConversionClass (display, "stringToOlFont", (String)0);
        static XFontSet         fset;
	static XFontStruct	*font_struct;
        Screen *                screen;
	static OlStrRep		text_format;
	Boolean 		success;


        if (*num_args != 2)
                ConversionError (
                     "wrongParameters",
                     "String to OlFont conversion needs two args",
                     (String *)NULL,
                     (Cardinal *)NULL); /* NOT REACHED */

        screen = *((Screen **)(args[0].addr));
	text_format = *((OlStrRep *)(args[1].addr)); 
	switch(text_format) {
		case OL_SB_STR_REP:
			if(to->addr != NULL && to->size < 
						sizeof(XFontStruct *)) {
			to->size = sizeof(XFontStruct *);
			return(False);
			}
			
			if(to->addr == NULL) 
				to->addr = (XtPointer)&font_struct;
			
			to->size = sizeof(XFontStruct *);
				 
			success = XtCallConverter(display,
					CvtStringToFontStruct,
					(XrmValuePtr)&args[0],
					1,
					from,
					to,
					(XtCacheRef *)NULL);
			break;	
		case OL_MB_STR_REP:
		case OL_WC_STR_REP:
			if(to->addr != NULL && to->size < 
						sizeof(XFontSet)) {
			to->size = sizeof(XFontSet);
			return(False);
			}
			
			if(to->addr == NULL) 
				to->addr = (XtPointer)&fset;
			
			to->size = sizeof(XFontSet);
				 
			success = XtCallConverter(display,
					CvtStringToFontSet,
					(XrmValuePtr)&args[0],
					1,
					from,
					to,
					(XtCacheRef *)NULL);
			break;
		default:
			success = False;
			XtDisplayStringConversionWarning (display, 
							from->addr, 
							XtROlFont);
			OlError(dgettext(OlMsgsDomain,
"Unrecognizable text format passed to \
String to OlFont Converter: text format resource occurs after font resource \
in the resource list of passed widget?\n"));
			break;
			
	}
return(success);
					
} /* CvtStringToOlFontPrivate */
 
 
/**
 ** IsFontSet()
 **/
 
static Boolean
IsFontSet (
        String                  who,
        Screen *                screen,
        String                  str,
        XFontSet *              p_fontset,
        XtPointer *             p_converter_data
)
{
        XrmRepresentation       type;
        XrmValue                value;
 
        Display *               display = DisplayOfScreen(screen);
        String                  try;
	String 			defaultFont;
 
#define  p_free_the_fontset (Boolean *)p_converter_data
 
        /*
         * Assume we'll allocate the fontset.
         */
        *p_free_the_fontset = True;
           
	/*
	 * If we were given a non-null base font name list other than
	 * the XtDefaultFont, XtDefaultFontSet OR the 
         * OlDefaultBoldFont. Try fetching first.
	 */

	if (str && *str && !CASELESS_STREQU(str, XtDefaultFont)
			&& !CASELESS_STREQU(str, XtDefaultFontSet)
			&& !CASELESS_STREQU(str, 
					(String)OlDefaultBoldFont)) {
		try = MapFontSet(screen,str,False, False, False);
		if (*p_fontset = LoadFontSet(display, try, False))
			return (True);
		XtDisplayStringConversionWarning (display, str, who);
	}

	/*
	 * Either we were asked to fetch the XtDefaultFontSet or
	 * OlDefaultBoldFont or XtDefaultFont or
	 * the font set requested isn't known by the X server. Try
	 * the XtDefaultFontSet first.
	 */

	if (FetchResource(display, "xtDefaultFontSet", "XtDefaultFontSet", 
							&type, &value)) {
		if (type == XrmPermStringToQuark(XtRString)) { 
			*p_fontset = LoadFontSet(display, 
						(String)value.addr, False);
			if (*p_fontset)
				return (True);
			else {
				XtDisplayStringConversionWarning (
					display,
					(String)value.addr,
					"FontSet"
				);
			}

		} else if (type == XrmPermStringToQuark(XtRFontSet)) {
			*p_fontset = *(XFontSet *)value.addr;
			*p_free_the_fontset = False;
			return (True);

		}	
	 }
	/*
	 * If a base font name  wasn't specified and the  xtDefaultFontset
	 * resource hasn't been set, use the Olit Default Font.
	 *
	 * The Olit default font has been "resource-ified" thru the
         * application resource "XtNolDefaultFont". It is important
         * that this resource be specified in XLFD format - for all
         * the following processing to occur correctly. The default
         * for this is a scalable 75x75 font - might want to change it
         * if the hand tuned fonts are changed to the more appropriate
         * 83x83. 
         * If the resolution fields are "*" or "?" , we assume that the
         * user wants resolution-independent fonts; we provide it based
         * on the screen's H/V resolutions.  Else we retain the
         * specified values. The Bold font is obtained by replacing the
         * WEIGHT_NAME field with "Bold".
         * We try to load the processed XLFD name . If this fails, we
         * assume that the server does'nt support scalable fonts.
         * 
	 */

	defaultFont = _OlGetOlDefaultFont(screen);
	if (CASELESS_STREQU(str, (String)OlDefaultBoldFont)) {
		/* try resolution independent first */

		try = MapFontSet(screen,defaultFont,True, True, True);
		if ((*p_fontset = LoadFontSet(display, try, False)))
			return (True);

		try = MapFontSet(screen,defaultFont,False,True,True);
		if ((*p_fontset = LoadFontSet(display, try, False)))
			return (True);
	}

	/* try to load the NOT bold font */
		/* try resolution independent first */

	try = MapFontSet(screen,defaultFont, True, True,False);
	if ((*p_fontset = LoadFontSet(display, try, False)))
		return (True);

	try = MapFontSet(screen,defaultFont,False, True,False);
	if ((*p_fontset = LoadFontSet(display, try, False)))
		return (True);

	{ /* Try to fetch the font name list from the localized file */
	String font_name_list = NULL;

	font_name_list = XtNewString(dgettext(OlMsgsDomain,
						"OL_BASE_FONT_NAME_LIST"));
	if(strcmp(font_name_list,"OL_BASE_FONT_NAME_LIST")) {
		try = MapFontSet(screen, font_name_list, True, True, False);
		if(font_name_list)
			XtFree(font_name_list);
		if ((*p_fontset = LoadFontSet(display, try, False)))
			return(True);
	} else
		if(font_name_list)
			XtFree(font_name_list);
	}

	/*
	 * Last resort is to find any reasonable fontset.
	 */
	*p_fontset = LoadFontSet(display,"-*-*-*-R-*-*-*-120-*-*-*-*-ISO8859-1,-*-*-*-R-*-*-*-120-*-*-*-*-*-*", True);
	if (*p_fontset)
		return (True);

	return (False);
 
#undef p_free_the_fontset
#undef display
} /* IsFontSet */
/**
 ** MapFontSet()
 **/

/*TOCHANGE*/
static String
MapFontSet (Screen *screen, String str,
	    Boolean resFlag, Boolean psFlag, Boolean boldFlag)
{

String start = str;
String end = str;
static String new = NULL;
String temp = NULL;
static char	copy[_XLFD_MAX_LENGTH + 1];
size_t len =0;
size_t prevlen = 0;
size_t slen = 0;


	for(; *end != '\0'; start=end) {

		for(; *end != ',' && *end != '\0'; end++)
				;

		len = (size_t)(end - start);

		if(len > _XLFD_MAX_LENGTH)
			return(str);

		strncpy(copy,(const char *)start,len);
		copy[len] = '\0';

		temp = ParseXLFDName(screen, copy, 
				resFlag, psFlag, boldFlag);

		slen = strlen((const char *)temp);

		new = XtRealloc(new,(sizeof(char)*
						(slen + prevlen +1)));

		strncpy((new+prevlen),(const char *)temp,slen);

		prevlen += slen;

		/* copy the coma or null character*/
		new[prevlen++] = *end;

		/* step beyond the coma */
		if(*end)
		end++;

		/* skip whitespace after a coma */
		while(isspace((int)*end))
			end++;
	}

	return(new);
	
} /* MapFontSet */
   
/**
 ** LoadFontSet()
 **/

static XFontSet
LoadFontSet (
        Display *               display,
        String                  name,
	Boolean			missing_charset_ok
)
{
        XFontSet                        f       = NULL;
        char ** miss_chset_list;
        int             miss_chset_cnt;
        char *  default_string;

        PrevErrorHandler = XSetErrorHandler(IgnoreBadName);

        GotBadName = False;


        f = XCreateFontSet(display, name,
                        &miss_chset_list, &miss_chset_cnt,
                        &default_string);

	if (miss_chset_cnt) { 
		int i;
		for(i=0; i < miss_chset_cnt; i++)
#ifdef DEBUG
			fprintf(stderr,"CvtStringToFontSet:\
missing charset: %s\n", miss_chset_list[i]);
#endif /* DEBUG */
                  XFreeStringList(miss_chset_list);
		if (!missing_charset_ok)
		    f = (XFontSet)NULL;  /* set f to NULL */
#ifdef DEBUG
		else
		    fprintf(stderr, "missing some charsets\n");
#endif /* DEBUG */
        }


        if (f == (XFontSet)NULL)
            GotBadName = True;
        XFlush (display);
        XSync (display, False);

        if (GotBadName)
                f = 0;

        (void)XSetErrorHandler (PrevErrorHandler);

        return (f);
} /* LoadFontSet */


static Boolean
CvtStringToFontSet (Display *display, XrmValue *args, Cardinal *num_args, XrmValue *from, XrmValue *to, XtPointer *converter_data)
{


        DeclareConversionClass (display, "stringToOlFont", (String)0);

        XFontSet                fset;
        Screen *                screen;
        XFontStruct *           fs;
        Font                    f;


        if (*num_args != 1)
                ConversionError (
                        "wrongParameters",
                        "String to FontStruct conversion needs Screen* argument",
                        (String *)0,
                        (Cardinal *)0
                );
                /*NOTREACHED*/
        screen = *((Screen **)(args[0].addr));

        if (!IsFontSet("FontSet", screen, (String)from->addr, &fset, converter_data))
                ConversionError (
                        "noFontSet",
                        "Unable to load any usable fontset",
                        (String *)0,
                        (Cardinal *)0
                );
 
        ConversionDone (XFontSet, fset);
}

static void
FreeFontSet (XtAppContext app, XrmValuePtr to, XtPointer closure, XrmValuePtr args, Cardinal *num_args)
{
	DeclareDestructionClass(app, "freeFontStruct", (String)0);
	Screen*			screen;

	#define free_the_font (Boolean)closure

	if (*num_args != 1)
		DestructionError("wrongParameters",
			"Free FontSet needs Screen* argument", (String*)0,
			(Cardinal*)0);
		/*NOTREACHED*/

	screen = *((Screen**)(args[0].addr));

	if (free_the_font)
		XFreeFontSet(DisplayOfScreen(screen), *(XFontSet*)to->addr);

	return;

	#undef free_the_font
} /* FreeFontStruct */

/*
 * This routine is used to set the converters for OlString and OlFont to 
 * use different converter args, this is because the vendor resources are
 * in the extension rec
 * Based on the Value of 'flag'
 *    True - Then set for Vendor Shell
 *	  False - set for others.
 */
extern void
OlVendorSetConverters(Boolean flag)
{
	if(flag) {
		XtSetTypeConverter (
			XtRString,
			XtROlFontPrivate,
			CvtStringToOlFontPrivate,
			vendorOlFontPrivateArgs,
			XtNumber(vendorOlFontPrivateArgs),
			XtCacheNone,
			NULL
		);
	}
	else {
		XtSetTypeConverter (
			XtRString,
			XtROlFontPrivate,
			CvtStringToOlFontPrivate,
			convertOlFontPrivateArgs,
			XtNumber(convertOlFontPrivateArgs),
			XtCacheNone,
			NULL
		);
	}
}

static String
ParseXLFDName(Screen* screen, 
		String str, 
		Boolean resFlag, 
		Boolean psFlag,
		Boolean boldFlag)
{
#define XLFD_WEIGHT_NAME         3
#define XLFD_PIXEL_SIZE		 7
#define XLFD_POINT_SIZE		 8
#define XLFD_RESOLUTION_X	 9
#define XLFD_RESOLUTION_Y	10
#define _XLFD_MAX_ELEMENT	14
#define XFNDelim		'-'
#define ASTERISK		'*'
#define QUESTION_MARK		'?'

	Dimension	H;
	Dimension	V;
	String		xlfd_parts[_XLFD_MAX_ELEMENT + 1];
	Cardinal	i;
	static char	copy[_XLFD_MAX_LENGTH + 1];
	String		p = copy;
	Cardinal	n = _XLFD_MAX_ELEMENT + 1;
	Cardinal	bytes;

	/*
	 * All legal XLFD names are limited in length.
	 */
	if (Strlen(str) > _XLFD_MAX_LENGTH)
		return (str);

	/*
	 * Parse the string into it's XLFD components.
	 * Note: No null terminators, please, as we don't own the
	 * string buffer.
	 */
	{
		register String		p;
		register String		part;
		register Cardinal	n	= _XLFD_MAX_ELEMENT + 1;

		p = str-1;	/* -1 because of ++ inside loop */
		i = 0;
		do {
			part = ++p;
			p = strchr(p, XFNDelim);
			xlfd_parts[i] = part;
		} while (++i < n && p);

		/*
		 * All legal XLFD names have a fixed number of components.
		 */
		if (i != _XLFD_MAX_ELEMENT + 1)
			return (str);
	}

	/*
	 * If the resolution flag is set and
	 * the resolution information is present,
	 * assume the client or user doesn't want us to
	 * convert the name.
	 */
	if (resFlag) {
		switch (*(xlfd_parts[XLFD_RESOLUTION_X])) {
		case XFNDelim:
		case ASTERISK:
		case QUESTION_MARK:
			break;
		default:
			resFlag = False;
			break;
		}
		switch (*(xlfd_parts[XLFD_RESOLUTION_Y])) {
		case XFNDelim:
		case ASTERISK:
		case QUESTION_MARK:
			break;
		default:
			resFlag = False;
			break;
		}
	}

	/*
	 * If the point size flag is set and
	 * the point size information is present,
	 * assume the client or user doesn't want us to
	 * convert the name.
	 */
	if (psFlag) {
		switch (*(xlfd_parts[XLFD_POINT_SIZE])) {
		case XFNDelim:
		case ASTERISK:
		case QUESTION_MARK:
			break;
		default:
			psFlag = False;
			break;
		}

		switch (*(xlfd_parts[XLFD_PIXEL_SIZE])) {
		case XFNDelim:
		case ASTERISK:
		case QUESTION_MARK:
			break;
		default:
			psFlag = False;
			resFlag = False;
			break;
		}
	}

	/*
	 * Calculate the screen resolution from the Screen structure.
	 */
#define DPI(DOTS,MM)	(Cardinal)( ((DOTS) * 254) / ((MM) * 10) )
	H = DPI(WidthOfScreen(screen), WidthMMOfScreen(screen));
	V = DPI(HeightOfScreen(screen), HeightMMOfScreen(screen));

	/*
	 * Copy the font name replacing the point size and 
	 * resolution fields with the optimum values.
	 *
	 * Skip the font name registry, we assume it's null.
	 */
	*p++ = XFNDelim;
	for (i = 1; i < n; i++) {
		if (psFlag && (i == XLFD_POINT_SIZE)) {
			(void) sprintf(p, "%d0-", _OlGetScale(screen));
			p += strlen(p);
		} else if (resFlag && (i == XLFD_RESOLUTION_X)) {
			(void) sprintf(p, "%d-", H);
			p += strlen(p);
		} else if (resFlag && (i == XLFD_RESOLUTION_Y)) {
			(void) sprintf(p, "%d-", V);
			p += strlen(p);
		} else if (boldFlag && (i == XLFD_WEIGHT_NAME)) {
			(void) sprintf(p, "Bold-");
			p += 5;
		} else if (i == _XLFD_MAX_ELEMENT) {
			strcpy (p, xlfd_parts[i]);
		} else {
			bytes = xlfd_parts[i+1] - xlfd_parts[i];
			strncpy (p, xlfd_parts[i], bytes);
			p += bytes;
		}
	}

	return (copy);

} /* MapFont */

/*
 * CvtStringToOlEditMode
 *
 */

static          Boolean
CvtStringToOlEditMode(Display *dpy, 
			XrmValue *args, 
			Cardinal *num_args, 
			XrmValue *from, 
			XrmValue *to, 
			XtPointer *converter_data)
{
    static OlEditMode mode;
    OlEditMode     *mp;
    char           *p = from->addr;

    if (0 == strcmp(p, "textread") || 0 == strcmp(p, "oltextread"))
	mode = OL_TEXT_READ;
    else if (0 == strcmp(p, "textedit") || 0 == strcmp(p, "oltextedit"))
	mode = OL_TEXT_EDIT;
    else {
	XtDisplayStringConversionWarning(dpy, from->addr, "EditMode");
	return False;
    }

    mp = (OlEditMode *) (to->addr);
    if (mp == (OlEditMode *) NULL || to->size != sizeof(OlEditMode)) {
	to->addr = (caddr_t) & mode;
	to->size = sizeof(OlEditMode);
    } else {
	*mp = mode;
    }

    return (True);
}				/* end of EditModeConverter */

 
static Boolean
CvtStringToOlScale(Display *display, XrmValue *args, Cardinal *num_args, 
		   XrmValue *from, XrmValue *to, XtPointer *converter_data)
{
        DeclareConversionClass(display, "stringToOlScale", (String)0);
        int                     i;
        NameValue *            	p;

        if (*num_args != 0)
                ConversionError (
                        "wrongParameters",
                        "String to OlScale conversion needs no arguments",
                        (String *)0,
                        (Cardinal *)0
                );
                /*NOTREACHED*/

	/* Search for string matches first */
        for (p = scales; p->name; p++)
                if (CASELESS_STREQU((String)from->addr, p->name)) {
			i = p->value;
                        break;
		}

	if (!p->name) {
		/* No string match was found, assume value is an int */

           	if (!IsInteger((String)from->addr, &i) || i < 0) {
                	static String           _args[2]  = { 0 , 0 };
                	Cardinal                _num_args = 1;

                	_args[0] = (String)from->addr;
                	ConversionWarning (
                        	"illegalString",
                        	"String to OlScale given illegal string: %s",
                        	_args,
                        	&_num_args
                	);
			return (False);
        	}
	}
        ConversionDone (int, (int)i);

} /* CvtStringToOlScale() */
