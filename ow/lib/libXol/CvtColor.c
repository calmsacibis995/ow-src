#pragma ident	"@(#)CvtColor.c	302.6	97/03/26 lib/libXol SMI"	/* olmisc:CvtColor.c 1.2	*/

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

#include <ctype.h>
#include <libintl.h>
#include <stdio.h>
#include <string.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>

#include <Xol/Converters.h>
#include <Xol/OlI18nP.h>
#include <Xol/OpenLookP.h>
#include <Xol/Util.h>


/*
 * Local routines:
 */

static Boolean		StringToColorTupleList (
	Display *		display,
	XrmValue *		args,
	Cardinal *		num_args,
	XrmValue *		from,
	XrmValue *		to,
	XtPointer *		converter_data
);
static void		FreeColorTupleList (
	XtAppContext		app,
	XrmValuePtr		to,
	XtPointer		closure,	/* unused */
	XrmValuePtr		args,
	Cardinal		*num_args
);
static Boolean		IsColorTupleList (
	String			_input,
	OlColorTupleList **	p_list,
	XrmValue *		args,
	Cardinal *		num_args
);
static Boolean		ParseColorTupleList (
	String			_input,
	Cardinal *		p_ntuples,
	OlColorTuple *		store,
	XrmValue *		args,
	Cardinal *		num_args
);
static XtTypeConverter	__OlFindConverter (
	Display *		display,
	XrmQuark		from,
	XrmQuark		to
);
static String		strbal (
	String			str,
	char			left,
	char			right
);

#if	defined(NEED_STRING_TO_PIXEL)
static Boolean		CvtStringToPixel (
	Display *		display,
	XrmValue *		args,
	Cardinal *		num_args,
	XrmValue *		from,
	XrmValue *		to,
	XtPointer *		converter_data
);
static void		FreePixel (
	XtAppContext		app,
	XrmValuePtr		to,
	XtPointer		closure,	/* unused */
	XrmValuePtr		args,
	Cardinal		*num_args
);
#endif

/*
 * Global data:
 */

XtTypeConverter	_OlCvtStringToColorTupleList = StringToColorTupleList;

/**
 ** OlRegisterColorTupleListConverter()
 **/

void
OlRegisterColorTupleListConverter (void)
{
	/*
	 * Look in Xt/Converters.c.
	 */
	static Boolean		registered	= False;


	if (!registered) {
		XtSetTypeConverter (
			XtRString,
			XtROlColorTupleList,
			StringToColorTupleList,
			(XtConvertArgList)colorConvertArgs,
			2,
			XtCacheByDisplay,
			FreeColorTupleList
		);
		registered = True;
	}
	return;
}

/**
 ** StringToColorTupleList()
 **/

/*ARGSUSED5*/
static Boolean
StringToColorTupleList (
	Display *		display,
	XrmValue *		args,
	Cardinal *		num_args,
	XrmValue *		from,
	XrmValue *		to,
	XtPointer *		converter_data
)
{
	DeclareConversionClass(display, "stringToColorTupleList", (String)0);

	OlColorTupleList *	list;

	Screen *		screen;

	Colormap		colormap;


	if (*num_args != 2)
		ConversionError (
			"wrongParameters",
			"String to ColorTupleList conversion needs Screen* and Colormap arguments",
			(String *)0,
			(Cardinal *)0
		);
		/*NOTREACHED*/

	screen   =  *((Screen **)(args[0].addr));
	colormap = *((Colormap *)(args[1].addr));

	if (!IsColorTupleList((String)from->addr, &list, args, num_args)) {
		static String		_args[2]  = { 0 , 0 };
		static Cardinal		_num_args = 1;

		_args[0] = (String)from->addr;
		ConversionWarning (
			"illegalString",
			"String to ColorTupleList given illegal string: %s",
			_args,
			&_num_args
		);
		return (False);
	}

	ConversionDone (OlColorTupleList *, list);
} /* StringToColorTupleList */

/**
 ** FreeColorTupleList()
 **/

static void
FreeColorTupleList (
	XtAppContext		app,
	XrmValuePtr		to,
	XtPointer		closure,	/* unused */
	XrmValuePtr		args,
	Cardinal *		num_args
)
{
	DeclareDestructionClass(app, "freeColorTupleList", (String)0);

	OlColorTupleList *	p;


	if (*num_args != 2)
		DestructionError (
			"wrongParameters",
			"Free ColorTupleList needs Screen* and Colormap arguments",
			(String *)0,
			(Cardinal *)0
		);
		/*NOTREACHED*/

	p = *(OlColorTupleList **)to->addr;
	XtFree((char *)p->list);
	XtFree((char *)p);

	return;
}

/**
 ** IsColorTupleList()
 **/

static Boolean
IsColorTupleList (
	String			_input,
	OlColorTupleList **	p_list,
	XrmValue *		args,
	Cardinal *		num_args
)
{
	OlColorTupleList *	p;

	Cardinal		ntuples;


	/*
	 * Two passes over the input, the first to count the
	 * number of tuples, the second to store the tuples.
	 * Sounds inefficient, but it is better than malloc'ing
	 * on the fly.
	 */

#define PARSE(PN,L) ParseColorTupleList(_input,(PN),(L),args,num_args)

	if (!PARSE(&ntuples, (OlColorTuple *)0) || !ntuples)
		return (False);

	p = *p_list = XtNew(OlColorTupleList);
	p->size = ntuples;
	p->list = (OlColorTuple *)XtMalloc(ntuples * sizeof(OlColorTuple));
	if (!PARSE((Cardinal *)0, p->list))
		return (False);

	return (True);
} /* IsColorTupleList */

/**
 ** ParseColorTupleList()
 **/
		
static Boolean
ParseColorTupleList (
	String			_input,
	Cardinal *		p_ntuples,
	OlColorTuple *		store,
	XrmValue *		args,
	Cardinal *		num_args
)
{
	Display *		display;

	Cardinal		ntuples;

	String			input	= Strdup(_input);
	String			tuple;
	String			rest;

	static Cardinal		offset[4] = {
		XtOffsetOf(OlColorTuple, bg0),
		XtOffsetOf(OlColorTuple, bg1),
		XtOffsetOf(OlColorTuple, bg2),
		XtOffsetOf(OlColorTuple, bg3)
	};

	display	= DisplayOfScreen(*((Screen **)(args[0].addr)));


/*
 * Easy way to ensure we always free things before leaving:
 */
#define RETURN(R) return ((XtFree(input), (R)))


	/*
	 * Syntax:
	 *
	 *	list  := list , tuple
	 *	tuple := ( bg0 , bg1 , bg2 , bg3 )
	 *
	 * Each of bg0,...,bg3 is a value that can be parsed
	 * with the registered String to Pixel converter, optionally
	 * enclosed in parentheses. (Enclosing in parentheses allows
	 * giving a string that has arbitrary syntax parsed by a
	 * new String to Pixel routine added by a client.)
	 *
	 * For example:
	 *
	 *	(white, red, lightRed, darkRed), (#123, #234, #456, #567)
	 */

	for (
		tuple = strchr(input, LPAR), ntuples = 0;
		tuple;
		tuple = strchr(rest, LPAR),  ntuples++
	) {
		String			p;

		Cardinal		i;

		String			BGi;

		XrmValue		from;

		static Pixel		bgi;

		static XrmValue	to   = { sizeof(Pixel), (XtPointer)&bgi };


		tuple++;
		if (!(p = strbal(tuple, LPAR, RPAR)))
			RETURN (False);
		*p++ = 0;
		rest = p;

		for (i = 0; i <= 3 && tuple; i++) {

			while (isspace(*tuple))
				tuple++;
			if (!*tuple)
				RETURN (False);

			if (*tuple == LPAR) {
				BGi = tuple+1;
				while (isspace(*BGi))
					BGi++;
				if (!*BGi)
					RETURN (False);

				if (!(tuple = strbal(BGi, LPAR, RPAR)))
					RETURN (False);
				*tuple = 0;
			} else
				BGi = tuple;

			/*
			 * Prepare for the next pass; this will
			 * also let us terminate this pass' color
			 * value with a null.
			 */
			tuple = strchr(tuple+1, SEP);
			if (tuple)
				*tuple++ = 0;

			/*
			 * Trim trailing whitespace from the color value.
			 */
			for (p = strchr(BGi, 0); p != BGi && isspace(p[-1]); p--)
				;
			if (p == BGi)
				RETURN (False);
			*p = 0;

			if (store) {
				from.addr = BGi;
				from.size = Strlen(BGi) + 1;
				if (!XtCallConverter(
					display,
#if	!defined(XtSpecificationRelease) || XtSpecificationRelease < 5
					CvtStringToPixel,
#else
					XtCvtStringToPixel,
#endif
					args,
					*num_args,
					&from,
					&to,
					(XtCacheRef *)0
				))
					RETURN (False);

				*((Pixel *)((char *)store + offset[i])) = bgi;
			}
		}

		if (store)
			store++;
	}

	if (p_ntuples)
		*p_ntuples = ntuples;

	RETURN (True);

} /* ParseColorTupleList */

/**
 ** strbal()
 **/

static String
strbal (
	String			str,
	char			left,
	char			right
)
{
	/*
	 * Find the character that balances another character,
	 * ignoring nested pairs of such characters. For instance,
	 * given a pointer to the character AFTER a left parenthesis,
	 * this routine will find the balancing right parenthesis.
	 * It will skip enclosed, balanced pairs of parentheses.
	 */
	for (; *str && *str != right; str++)
		if (*str == left) {
			str = strbal(str+1, left, right);
			if (!str)
				return (0);
		}
	return (!*str? 0 : str);
}

/**
 ** CvtStringToPixel()
 **/

#if	defined(NEED_STRING_TO_PIXEL)

/*
 * Define some stuff to replace/mimic hidden data structures.
 */

#define toVal		to
#define done(T,V)	{ ConversionDone(T,V); }

typedef struct foo {
	XtAppContext	appContext;
	Boolean		rv;
}		*XtPerDisplay;

static XtPerDisplay
_XtGetPerDisplay (
	Display *		display
)
{
	static struct foo	ret;

	XrmName			xrm_name[2];

	XrmClass		xrm_class[2];

	XrmRepresentation	type;

	XrmValue		value;


	ret.appContext = XtDisplayToApplicationContext(display);

	ret.rv = False;
	xrm_name[0]  = XrmStringToName("reverseVideo");
	xrm_name[1]  = 0;
	xrm_class[0] = XrmStringToClass("ReverseVideo");
	xrm_class[1] = 0;
	if (XrmQGetResource(XtDatabase(display), xrm_name, xrm_class,  &type, &value)) {
		static int	CompareISOLatin1();

		if (type == XtQBoolean)
			ret.rv = (Boolean)value.addr;

		else if (type == XtQInt)
			ret.rv = ((int)value.addr != 0);

#define STR		(String)value.addr
#define STRSAME(A,B)	(CompareISOLatin1((A),(B)) == 0)
		else if (
			STRSAME(STR, "true")
		     || STRSAME(STR, "yes")
		     || STRSAME(STR, "on")
		     || STRSAME(STR, "1")
		)
			ret.rv = True;
#undef STR
#undef STRSAME

	}

	return (&ret);
}

/*
 *======================================================================
 *
 * Snarfed from Xt/Converters.c:
 */

#include	<X11/keysym.h>

static int CompareISOLatin1 (first, second)
    char *first, *second;
{
    register unsigned char *ap, *bp;

    for (ap = (unsigned char *) first, bp = (unsigned char *) second;
	 *ap && *bp; ap++, bp++) {
	register unsigned char a, b;

	if ((a = *ap) != (b = *bp)) {
	    /* try lowercasing and try again */

	    if ((a >= XK_A) && (a <= XK_Z))
	      a += (XK_a - XK_A);
	    else if ((a >= XK_Agrave) && (a <= XK_Odiaeresis))
	      a += (XK_agrave - XK_Agrave);
	    else if ((a >= XK_Ooblique) && (a <= XK_Thorn))
	      a += (XK_oslash - XK_Ooblique);

	    if ((b >= XK_A) && (b <= XK_Z))
	      b += (XK_a - XK_A);
	    else if ((b >= XK_Agrave) && (b <= XK_Odiaeresis))
	      b += (XK_agrave - XK_Agrave);
	    else if ((b >= XK_Ooblique) && (b <= XK_Thorn))
	      b += (XK_oslash - XK_Ooblique);

	    if (a != b) break;
	}
    }
    return (((int) *bp) - ((int) *ap));
}

static Boolean CvtStringToPixel(dpy, args, num_args, fromVal, toVal, closure_ret)
    Display*	dpy;
    XrmValuePtr args;
    Cardinal    *num_args;
    XrmValuePtr	fromVal;
    XrmValuePtr	toVal;
    XtPointer	*closure_ret;
{
    String	    str = (String)fromVal->addr;
    XColor	    screenColor;
    XColor	    exactColor;
    Screen	    *screen;
    XtPerDisplay    pd = _XtGetPerDisplay(dpy);
    Colormap	    colormap;
    Status	    status;
    String          params[1];
    Cardinal	    num_params=1;

    if (*num_args != 2)
     XtAppErrorMsg(pd->appContext,
		dgettext(OlMsgsDomain, "wrongParameters"),
		dgettext(OlMsgsDomain, "cvtStringToPixel"),
		dgettext(OlMsgsDomain, "XtToolkitError"),
		dgettext(OlMsgsDomain, "String to pixel conversion needs screen and colormap arguments"),
        (String *)NULL, (Cardinal *)NULL);

    screen = *((Screen **) args[0].addr);
    colormap = *((Colormap *) args[1].addr);

    if (CompareISOLatin1(str, XtDefaultBackground) == 0) {
	*closure_ret = False;
	if (pd->rv) done(Pixel, BlackPixelOfScreen(screen))
	else	    done(Pixel, WhitePixelOfScreen(screen));
    }
    if (CompareISOLatin1(str, XtDefaultForeground) == 0) {
	*closure_ret = False;
	if (pd->rv) done(Pixel, WhitePixelOfScreen(screen))
        else	    done(Pixel, BlackPixelOfScreen(screen));
    }

    if (*str == '#') {  /* some color rgb definition */

        status = XParseColor(DisplayOfScreen(screen), colormap,
			     (char*)str, &screenColor);

        if (status != 0)
           status = XAllocColor(DisplayOfScreen(screen), colormap,
                                &screenColor);
    } else  /* some color name */

        status = XAllocNamedColor(DisplayOfScreen(screen), colormap,
                                  (char*)str, &screenColor, &exactColor);
    if (status == 0) {
	params[0] = str;
	XtAppWarningMsg(pd->appContext,
		dgettext(OlMsgsDomain, "noColormap"),
		dgettext(OlMsgsDomain, "cvtStringToPixel"),
			
		dgettext(OlMsgsDomain, "XtToolkitError"),
                 
		dgettext(OlMsgsDomain, "Cannot allocate colormap entry for \"%s\""),
                  params,&num_params);
	return False;
    } else {
	*closure_ret = (char*)True;
        done(Pixel, screenColor.pixel);
    }
}

/* ARGSUSED */
static void FreePixel(app, toVal, closure, args, num_args)
    XtAppContext app;
    XrmValuePtr	toVal;
    XtPointer	closure;
    XrmValuePtr	args;
    Cardinal	*num_args;
{
    Screen	    *screen;
    Colormap	    colormap;

    if (*num_args != 2)
     XtAppErrorMsg(app,
	dgettext(OlMsgsDomain, "wrongParameters"),
	dgettext(OlMsgsDomain, "freePixel"),
	dgettext(OlMsgsDomain, "XtToolkitError"),
	dgettext(OlMsgsDomain, "Freeing a pixel requires screen and colormap arguments"),
        (String *)NULL, (Cardinal *)NULL);

    screen = *((Screen **) args[0].addr);
    colormap = *((Colormap *) args[1].addr);

    if (closure) {
	XFreeColors( DisplayOfScreen(screen), colormap,
		     (unsigned long*)toVal->addr, 1, (unsigned long)0
		    );
    }
}

#endif
