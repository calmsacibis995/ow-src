#pragma ident	"@(#)StaticText.c	302.21	97/03/26 lib/libXol SMI"	/* statictext:src/StaticText.c 1.53 */

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

/*************************************<+>*************************************
 *****************************************************************************
 **
 **		File:        StaticText.c
 **
 **		Project:     X Widgets
 **
 **		Description: Code/Definitions for StaticText widget class.
 **
 *****************************************************************************
 **   
 **   Copyright (c) 1988 by Hewlett-Packard Company
 **   Copyright (c) 1988 by the Massachusetts Institute of Technology
 **   
 **   
 *****************************************************************************
 *************************************<+>*************************************/


#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <widec.h>
#include <libintl.h>

#include <X11/Xatom.h>
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <X11/keysymdef.h>

#include <Xol/OlI18nP.h>
#include <Xol/OlStrMthdsI.h>
#include <Xol/OpenLookP.h>
#include <Xol/RootShell.h>
#include <Xol/StaticTexP.h>
#include <Xol/buffutil.h>
#include <Xol/memutil.h>

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/

static void	StStartSelection (StaticTextWidget stw, unsigned char *position, Boolean motion);
static void     StFetchSelection (StaticTextWidget stw, unsigned char **start, unsigned char **finish);
static void	StExtendSelection (StaticTextWidget stw, unsigned char *position);
static void	SelectChar(unsigned char *inputstart, unsigned char *inputend, unsigned char **start, unsigned char **end, unsigned char *stringstart);
static void	SelectWord(unsigned char *inputstart, unsigned char *inputend, unsigned char **start, unsigned char **end, unsigned char *stringstart);
static void	SelectLine(unsigned char *inputstart, unsigned char *inputend, unsigned char **start, unsigned char **end, unsigned char *stringstart);
static void	SelectAll(unsigned char *inputstart, unsigned char *inputend, unsigned char **start, unsigned char **end, unsigned char *stringstart);
static void	StAlterSelection (StaticTextWidget stw, int mode);
static void	StHighlightSelection(StaticTextWidget stw, Boolean forceit);
static void	StXYForPosition(StaticTextWidget stw, unsigned char *position, Position *px, Position *py);
static Position	StGetOffset(StaticTextWidget stw, int line);
static unsigned char	*StPositionForXY(StaticTextWidget stw, Position x, Position y, Boolean at_start);
static void	StStartAction(StaticTextWidget stw, XEvent *event);
static void	StEndAction(StaticTextWidget stw);
static void	StDisplaySubstring(StaticTextWidget stw, unsigned char *start, unsigned char *finish, GC gc, Boolean isSensitive);
static void	StBuildLineTable(StaticTextWidget stw);
static void	StPasteTextOnClipBoard();
static void	WriteToCB(StaticTextWidget stw);
static void	ValidateInputs(StaticTextWidget stw);
static void	SetUpGCs(StaticTextWidget stw, Boolean isSensitive);
static int	RetCount(OlStrRep tf, OlStr string);
static Dimension	Maxline(StaticTextWidget stw);
static void	FormatText(StaticTextWidget stw);
static void	GetTextRect(StaticTextWidget newstw);
static void	SetSize(StaticTextWidget newstw);
static void 	StripSpaces(StaticTextWidget stw);


					/* class procedures		*/

static void	Initialize(StaticTextWidget req, StaticTextWidget new, ArgList args, Cardinal *p_num_args);
static void	Realize(StaticTextWidget stw, XtValueMask *valueMask, XSetWindowAttributes *attributes);
static void	Destroy(StaticTextWidget stw);
static void	Resize(StaticTextWidget stw);
static Boolean	SetValues(StaticTextWidget current, StaticTextWidget request, StaticTextWidget new, ArgList args, Cardinal *num_args);
static void	Redisplay(StaticTextWidget stw);
static void	ClassInitialize(void);

					/* action procedures		*/
static void	SelectStart(StaticTextWidget stw, XEvent *event);
static void	SelectAdjust(StaticTextWidget stw, XEvent *event);


static void		(*select_table[])() = {
	SelectChar, SelectWord, SelectLine, SelectAll, NULL
};

#define	NUM_SELECTION_TYPES	4

static OlDefine	alignment = OL_LEFT;
static OlStrRep text_format;
static size_t wcsiz = sizeof(wchar_t);
static const wcnull = L'\0';

/*************************************<->*************************************
 *
 *
 *	Description:  default translation table for class: StaticText
 *	-----------
 *
 *	Matches events with string descriptors for internal routines.
 *
 *************************************<->***********************************/

static char defaultTranslations [] = "\
	<FocusIn>:	OlAction() \n\
	<FocusOut>:	OlAction() \n\
	<Key>:		OlAction() \n\
	<BtnDown>:	OlAction() \n\
	<BtnUp>:	OlAction() \n\
\
	<BtnMotion>:	OlAction()";

/*************************************<->*************************************
 *
 *
 *	Description:  action list for class: StaticText
 *	-----------
 *
 *	Matches string descriptors with internal routines.
 *
 *************************************<->***********************************/
/* this widget doesn't have an action list */

static Boolean	ActivateWidget (Widget, OlVirtualName, XtPointer);
static void	HandleButton(Widget w, OlVirtualEvent ve);
static void	HandleMotion(Widget w, OlVirtualEvent ve);
static void	TakeFocus(Widget w, XEvent *event);

static OlEventHandlerRec event_procs[] =
{
	{ ButtonPress,		HandleButton	},
	{ ButtonRelease,	HandleButton	},
	{ MotionNotify,		HandleMotion	}
};

/*************************************<->*************************************
 *
 *
 *	Description:  resource list for class: StaticText
 *	-----------
 *
 *	Provides default resource settings for instances of this class.
 *	To get full set of default settings, examine resouce list of super
 *	classes of this class.
 *
 *************************************<->***********************************/

static XtResource resources[] = {
	{ XtNhSpace,
		XtCHSpace,
		XtRDimension,
		sizeof(Dimension),
		XtOffset(StaticTextWidget, static_text.internal_width),
		XtRString,
		"2"
	},
	{ XtNvSpace,
		XtCVSpace,
		XtRDimension,
		sizeof(Dimension),
		XtOffset(StaticTextWidget, static_text.internal_height),
		XtRString,
		"2"
	},

	{ XtNalignment,
		XtCAlignment,
		XtROlDefine,
		sizeof(OlDefine),
		XtOffset(StaticTextWidget,static_text.alignment),
		XtROlDefine,
		(XtPointer) &alignment
	},

	{ XtNgravity,
		XtCGravity,
		XtRGravity,
		sizeof(int),
		XtOffset(StaticTextWidget,static_text.gravity),
		XtRImmediate,
		(XtPointer) CenterGravity
	},
	{ XtNwrap,
		XtCWrap,
		XtRBoolean,
		sizeof(Boolean),
		XtOffset(StaticTextWidget,static_text.wrap),
		XtRString,
		"TRUE"
	},
	{ XtNstrip,
		XtCStrip,
		XtRBoolean,
		sizeof(Boolean),
		XtOffset(StaticTextWidget,static_text.strip),
		XtRString,
		"TRUE"
	},
	{ XtNlineSpace,
		XtCLineSpace,
		XtRInt,
		sizeof(int),
		XtOffset(StaticTextWidget,static_text.line_space),
		XtRString,
		"0"
	},
	{ XtNstring,
		XtCString,
		XtROlStr,
		sizeof(OlStr),
		XtOffset(StaticTextWidget, static_text.input_string),
		XtRImmediate,
		(OlStr)NULL
	},
	{ XtNrecomputeSize,
		XtCRecomputeSize,
		XtRBoolean,
		sizeof(Boolean),
		XtOffset(StaticTextWidget,static_text.recompute_size),
		XtRString,
		"TRUE"
	},
	{ XtNselectable,
		XtCSelectable,
		XtRBoolean,
		sizeof(Boolean),
		XtOffset(StaticTextWidget, static_text.selectable),
		XtRImmediate,
		(XtPointer)False
	},
	{ XtNbackground,
		XtCBackground,
		XtRPixel,
		sizeof(Pixel),
		XtOffset(StaticTextWidget, core.background_pixel),
		XtRString,
		XtDefaultBackground
	},
	{ XtNtraversalOn,
		XtCTraversalOn,
		XtRBoolean,
		sizeof(Boolean),
		XtOffset(StaticTextWidget, primitive.traversal_on),
		XtRImmediate,
		(XtPointer)False
	}
};


/*************************************<->*************************************
 *
 *
 *	Description:  global class record for instances of class: StaticText
 *	-----------
 *
 *	Defines default field settings for this class record.
 *
 *************************************<->***********************************/

StaticTextClassRec statictextClassRec = {
	{ /* core_class fields */
	/* superclass            */	(WidgetClass) &(primitiveClassRec),
	/* class_name            */	"StaticText",
	/* widget_size           */	sizeof(StaticTextRec),
	/* class_initialize      */	ClassInitialize,
	/* class_part_initialize */	NULL,
	/* class_inited          */	FALSE,
	/* initialize            */	(XtInitProc) Initialize,
	/* initialize_hook       */	NULL,
	/* realize               */	(XtRealizeProc) Realize,
	/* actions               */	NULL,
	/* num_actions           */	0,
	/* resources             */	resources,
	/* num_resources         */	XtNumber(resources),
	/* xrm_class             */	NULLQUARK,
	/* compress_motion       */	TRUE,
	/* compress_exposure     */	TRUE,
	/* compress_enterleave   */	TRUE,
	/* visible_interest      */	FALSE,
	/* destroy               */	(XtWidgetProc) Destroy,
	/* resize                */	(XtWidgetProc) Resize,
	/* expose                */ 	(XtExposeProc) Redisplay,
	/* expose                	(XtExposeProc) NULL,*/
	/* set_values            */	(XtSetValuesFunc) SetValues,
	/* set_values_hook       */	NULL,
	/* set_values_almost     */	(XtAlmostProc) XtInheritSetValuesAlmost,
	/* get_values_hook       */	NULL,
	/* accept_focus          */	XtInheritAcceptFocus,
	/* version               */	XtVersion,
	/* callback private      */	NULL,
	/* tm_table              */	defaultTranslations,
	/* query_geometry        */	NULL,
	},
  {					/* primitive class	*/
      NULL,				/* reserved		*/
      XtInheritHighlightHandler,	/* highlight_handler	*/
      NULL,				/* traversal_handler	*/
      NULL,				/* register_focus	*/
      ActivateWidget,			/* activate		*/ 
      event_procs,			/* event_procs		*/
      XtNumber(event_procs),		/* num_event_procs	*/
      OlVersion,			/* version		*/
      NULL,				/* extension		*/
      NULL,				/* transparent_proc	*/
      NULL,				/* query_sc_locn_proc   */
  },
	{
		NULL,
	},
};

WidgetClass statictextWidgetClass = (WidgetClass) &statictextClassRec;
WidgetClass staticTextWidgetClass = (WidgetClass) &statictextClassRec;

/*************************************<->*************************************
 *
 *  ClassInitialize
 *
 *   Description:
 *   -----------
 *    Set up translations.
 *
 *************************************<->***********************************/
static void ClassInitialize(void)
{
	_OlAddOlDefineType ("center", OL_CENTER);
	_OlAddOlDefineType ("left",   OL_LEFT);
	_OlAddOlDefineType ("right",  OL_RIGHT);
}

/*************************************<->*************************************
 * 
 *	Procedures and variables private to StaticText
 * 
 *************************************<->************************************/

/*************************************<->*************************************
 *
 *  static void ValidateInputs(stw)
 *		StaticTextWidget	stw;
 *
 *	Description:
 *	-----------
 *		Checks a StaticText widget for nonsensical values, and changes 
 *		nonsense values into meaninful values.
 *
 *	Inputs:
 *	------
 *
 *	Outputs:
 *	-------
 *
 *	Procedures Called
 *	-----------------
 *		OlWarning
 *
 *************************************<->***********************************/
static void ValidateInputs(StaticTextWidget stw)
{
	StaticTextPart	*stp;

	stp = &(stw->static_text);

	/* Check internal spacing */
	if (stp->internal_height < 0)
	{
		OlWarning(dgettext(OlMsgsDomain,
			"StaticTextWidget: internal_height was less than zero, set to zero\n"));
		stp->internal_height = 0;
	}
	if (stp->internal_width < 0)
	{
		OlWarning(dgettext(OlMsgsDomain,
			"StaticTextWidget: internal_width was less than zero, set to zero\n"));
		stp->internal_width = 0;
	}
	
	/* 
	 *Check line_spacing.  We will allow text to over write,
	 * we will not allow it to move from bottom to top.
	 */
	if (stp->line_space < -100)
	{
		OlWarning(dgettext(OlMsgsDomain,
			"StaticTextWidget: line_space was less than -100, set to 0\n"));
		stp->line_space = 0;
	}

	/*
	 * Check Alignment
	 */
	
	switch(stp->alignment)
	{
		case OL_LEFT:
			break;
		case OL_CENTER:
			break;
		case OL_RIGHT:
			break;
			/* Valid values. */
		default:
			OlWarning(dgettext(OlMsgsDomain,
				"StaticTextWidget: Unknown alignment, set to OL_LEFT instead"));
			stp->alignment = OL_LEFT;
		break;
	}

}

/*************************************<->*************************************
 *
 * static void SetUpGCs (stw,isSensitive)
 *	StaticTextWidget stw;
 *	Boolean		 isSensitive;
 *
 *	Description:
 *	-----------
 *		Sets up GCs for the static text widget to write to.
 *		One normal, one highlighted.
 *
 *	Inputs:
 *	------
 *		stw = Points to an instance structure which has statictext.foreground
 *	   	   core.background_pixel and static_text.font appropriately
 *	   	   filled in with values.
 *
 *	Outputs:
 *	-------
 *		Initializes static_text.normal_GC, .hilite_GC
 *
 *	Procedures Called
 *	-----------------
 *		XtGetGC
 *		XtReleaseGC
 *
 *************************************<->***********************************/
static void SetUpGCs(StaticTextWidget stw, Boolean isSensitive)
{
	XGCValues	values;
	XtGCMask	valueMask;

        if (stw->static_text.normal_GC != (GC)NULL)
            XtReleaseGC((Widget)stw, stw->static_text.normal_GC);
        if (stw->static_text.hilite_GC != (GC)NULL)
            XtReleaseGC((Widget)stw, stw->static_text.hilite_GC);

/*
** Normal GC
*/

	if (stw->primitive.font_color != (Pixel) -1) {
		values.foreground = stw->primitive.font_color;
	}
	else {
		values.foreground = stw->primitive.foreground;
	}
	values.background = stw->core.background_pixel;

	if(stw->primitive.text_format == OL_SB_STR_REP){
		values.font	= ((XFontStruct *)
					(stw->primitive.font))->fid;
		valueMask = GCForeground | 
			    GCBackground | 
		 	    GCFont       | 
			    GCFunction;
	}
	else{
		/* In MB or WC case , GCFont will be set by DrawString routine */
		valueMask = GCForeground | GCBackground | GCFunction;
	}

	values.function	= GXcopy;

	if (!isSensitive) {
		valueMask |= (GCStipple | GCFillStyle);
		values.fill_style = FillStippled;
		values.stipple = stw->static_text.devData->inactiveStipple;
	}

	stw->static_text.normal_GC = XtGetGC((Widget)stw,
		valueMask,&values);
/*
** Highlighted GC
*/

	values.foreground = stw->core.background_pixel;
	if (stw->primitive.font_color != (Pixel) -1)
		values.background = stw->primitive.font_color;
	else
		values.background = stw->primitive.foreground;

	if(stw->primitive.text_format == OL_SB_STR_REP){
		values.font	= ((XFontStruct *)
					(stw->primitive.font))->fid;
		valueMask = GCForeground | GCBackground | GCFont | GCFunction;
	}
	else{
		/* In MB or WC case , GCFont will be set by DrawString routine */
		valueMask = GCForeground | GCBackground | GCFunction;
	}
	values.function	= GXcopy;

	stw->static_text.hilite_GC = XtGetGC((Widget)stw,
		valueMask,&values);
}


/*************************************<->*************************************
 *
 *	Description:
 *		This routine returns the number of '\n' 
 *              characters in "string"
 *
 *	Inputs:
 *		string = The string to be counted.
 *
 *	Outputs:
 *		The number of '\n' characters in the string.
 *
 *	Procedures Called
 *
 *************************************<->***********************************/

static int 
RetCount(OlStrRep tf, OlStr string)
{
	int			numlines;
	int			len;
	wchar_t*		wstring;
	unsigned char*		cstring = (unsigned char*)string;

	switch (tf) {

	case OL_SB_STR_REP:
		numlines = (*cstring ? 1 : 0);
		while (*cstring) {
			if (*cstring == '\n')
				numlines++;
			cstring++;
		}
		break;

	case OL_MB_STR_REP:
		numlines = (*cstring ? 1 : 0);
		while ((len = mblen((char*)cstring, MB_CUR_MAX)) > 0) {
			if (len == 1 && (*cstring == '\n'))
				numlines++;
			cstring += len;
		}
		break;

	case OL_WC_STR_REP:
		wstring = (wchar_t*)string;
		numlines = (*wstring ? 1 : 0);
		while (*wstring) {
			if (*wstring == L'\n')
				numlines++;
			wstring++;
		}
		break;

	} /* switch */

	return (numlines);
}

/*************************************<->*************************************
 *
 *	static Dimension Maxline(stw)
 *			StaticTextWidget stw;
 *
 *	Description:
 *	-----------
 *		For a given string , returns the longest length of
 *		'\n' delimited series of characters.
 *
 *	Inputs:
 *	------
 *		stw : static text widget
 *
 *	Outputs:
 *	-------
 *		The maximum length in pixels of the longest 
 *		'\n' delimited series of characters in 
 *		stw->static_text.output_string
 *
 *	Procedures Called
 *	-----------------
 *		XTextWidth, XwcTextEscapement, XmbTextEscapement.
 *
 *************************************<->***********************************/
static Dimension 
Maxline(StaticTextWidget stw)
{
	int	i;
	Dimension	max = 0, cur = 0;
	unsigned char	*str1, *str2, *str3;
	wchar_t *wstr1, *wstr2, *wstr3;
	int	alignment = stw->static_text.alignment;
	XFontStruct	*font;
	XFontSet        fontset;
	StaticTextPart	*stp;
	int len, l,nspaces;

	switch(stw->primitive.text_format) {
		case OL_SB_STR_REP:
                	font = (XFontStruct *)stw->primitive.font;

			stp = &(stw->static_text);
			str1 = (unsigned char *)(stp->output_string);
			while(*str1) {
			if ((stp->strip) && 
				((alignment == OL_LEFT) || 
				(alignment == OL_CENTER)))
				while (*str1 == ' ')
					str1++;
			str2 = str1;
			for(i=0; ((*str1 != '\n') && *str1); i++, str1++);
			if ((stp->strip) && 
				((alignment == OL_RIGHT) || 
				(alignment == OL_CENTER))) {
				str3 = str1;
				str3--;
				while (*str3 == ' ')
					i--, str3--;
			}
			if(i)
				cur = (Dimension) XTextWidth(font,
						(const char *)str2,i);
			else
				cur =0;
			if (cur > max)
                       		 max = cur;
                	if (*str1)
                       		 str1++; /* past the \n */
			} /* end of while */
			break;
		case OL_WC_STR_REP:
                	fontset = (XFontSet)stw->primitive.font;

			stp = &(stw->static_text);
			wstr1 = (wchar_t *)(stp->output_string);
			while(*wstr1) {
			if ((stp->strip) && 
				((alignment == OL_LEFT) || 
				(alignment == OL_CENTER)))
				while (*wstr1 == L' ')
					wstr1++;
			wstr2 = wstr1;
			for(i=0; ((*wstr1 != L'\n') && 
						*wstr1); i++, wstr1++);
			if ((stp->strip) && 
				((alignment == OL_RIGHT) || 
				(alignment == OL_CENTER))) {
				wstr3 = wstr1;
				wstr3--;
				while (*wstr3 == L' ')
					i--, wstr3--;
			}

			if(i)
			    cur = (Dimension) 
					XwcTextEscapement(fontset,wstr2,i);
			else
				cur =0;
			if (cur > max)
                       		 max = cur;
                	if (*wstr1)
                       		 wstr1++; /* past the \n */
			} /* end of while */
			break;
		case OL_MB_STR_REP:
                	fontset = (XFontSet)stw->primitive.font;
			nspaces = 0;

			stp = &(stw->static_text);
			str1 = (unsigned char *)(stp->output_string);
			while((len = mblen((char *)str1,MB_CUR_MAX)) > 0) {
			if ((stp->strip) && 
				((alignment == OL_LEFT) || 
				(alignment == OL_CENTER)))
				while ((mblen((char *)str1,MB_CUR_MAX)
					 == 1) && *str1 == ' ')
					str1++;
			str2 = str1;
			for(i=0; ((l = mblen((char *)str1, MB_CUR_MAX)) 
				> 1 || (l == 1 &&  
				*str1 != '\n')); i+=l, str1+=l) 
				if(l == 1 && *str1 == ' ')
					nspaces++;
				else
					nspaces = 0;
			if ((stp->strip) && 
				((alignment == OL_RIGHT) || 
				(alignment == OL_CENTER))) 
					i-=nspaces; 
			if(i)
			    cur = (Dimension) 
					XmbTextEscapement(fontset,
						(const char *)str2,i);
			else
				cur =0;
			if (cur > max)
                       		 max = cur;
                	if (*str1)
                       		 str1++; /* past the \n */
			} /* end of while */
			break;
	} /* switch */
	return(max);
}

/*************************************<->*************************************
 *
 *	static void FormatText(stw)
 *		StaticTextWidget	stw;
 *
 *	Description:
 *	-----------
 *		Inserts newlines where necessary to fit 
 *               stw->static_text.output_string
 *		into stw->core.width (if specified).
 *
 *	Inputs:
 *	------
 *		stw = The StaticText widget to be formatted.
 *
 *	Outputs:
 *	-------
 *
 *	Procedures Called
 *	-----------------
 *		XTextWidth, XwcTextEscapement, XmbTextEscapement
 *
 *************************************<->***********************************/
static void 
FormatText(StaticTextWidget stw)
{
int	i, wordindex,len;
Dimension	width, win;
unsigned char	*str1, *str2, *save;
wchar_t	*wstr1, *wstr2;
XFontStruct *font;
XFontSet fontset;
Boolean	gotone;
StaticTextPart	*stp;

stp = &(stw->static_text);

/* The available text window width is... */
win = stw->core.width - (2 * stp->internal_width)
	- (2 * stw->static_text.highlight_thickness);

switch(stw->primitive.text_format) {
   case OL_SB_STR_REP:
	font = (XFontStruct *)(stw->primitive.font);
	str1 = (unsigned char *)(stp->output_string);
	while (*str1) {

		i = 0;
		width = 0;
		wordindex = -1;
		gotone = FALSE;
		str2 = str1;

		while (!gotone) {
			if ((stp->strip) && (wordindex == -1))
				while (*str1 == ' ')
					str1++;
			/*
			 *Step through until a character that we can
			 * break on.
			 */
			while ((*str1 != ' ') && 
			      (*str1 != '\n') && 
					(*str1))
				i++, str1++;

			wordindex++;

			width = (i ? XTextWidth(font,
						(const char *)str2,i) : 0);

			if (width < win) {

			/*
			 * If the current string fragment 
			 * is shorter than the 
			 * available window.  Check to see if we are at a 
			 * forced break or not, and process accordingly.
			 */

			switch (*str1) {
				case ' ':
				/*
				 * Add the space to the char count 
				 */
					i++;
				/*
				 * Check to see there's 
				 * room to start another
				 * word.
				 */

				width = XTextWidth(font, (const char *)str2,i);

				if (width >= win) {
				/*
				 * Break the line if we can't start
				 * another word.
				 */
					*str1 = '\n';
					gotone = TRUE;
				}
				/*
				 * Step past the space
				 */
				str1++;
				break;
				case '\n':
				/*
				 * Forced break.  Step pase the \n.
				 * Note the fall through to default.
				 */
					str1++;
				default:
				/*
				 * End of string.
				 */
					gotone = TRUE;
				break;
				} /* switch */

				} else if (width > win) {
			/*
			 * We know that we have something
			 */
					gotone = TRUE;

			/*
			 * See if there is at least one 
			 * space to back up for.
			 */
					if (wordindex) {
						str1--;
						while (*str1 != ' ')
							str1--;
						*str1++ = '\n';
					} else
				/*
				 * We have a single word which is too long
				 * for the available window.  Let the text
				 * clip rectangle handle it.
				 */
					if (*str1)
						*str1++ = '\n';
				} else /* (width == win) */ {
					switch (*str1) {
					case ' ':
					/*
					 * If we stopped on a space, 
					 * change it.
					 */
						*str1 = '\n';
					case '\n':
						/*
						 * Step past the \n.
						 */
						str1++;
					default:
						gotone = TRUE;
					break;
					} /* switch */
                        	} /* else */
		} /* while !gotone */
	} /* while *str1 */
	break;
   case OL_MB_STR_REP:
	str1 = (unsigned char *)(stp->output_string);
	fontset = (XFontSet)(stw->primitive.font);
	while (mblen((char *)str1,MB_CUR_MAX) > 0) {
		i = 0;
		width = 0;
		wordindex = -1;
		gotone = FALSE;
		str2 = str1;

		while (!gotone) {
			if ((stp->strip) && (wordindex == -1))
				while ((mblen((char *)str1,MB_CUR_MAX) 
					== 1) && *str1 == ' ')
					str1++;
			/*
			 *Step through until a character that we can
			 * break on.
			 */
			while (((len = mblen((char *)str1, MB_CUR_MAX))
				> 1) || (len == 1 && (*str1 != ' ') && 
			      		(*str1 != '\n') )) 
				i+=len, str1+=len;

			wordindex++;


			width = (i ? 
				XmbTextEscapement(fontset,
						  (const char *)str2,i) : 0);

			if (width < win) {

			/*
			 * If the current string fragment 
			 * is shorter than the 
			 * available window.  Check to see if we are at a 
			 * forced break or not, and process accordingly.
			 */

			switch (*str1) {
				case ' ':
				/*
				 * Add the space to the char count 
				 */
					i++;
				/*
				 * Check to see there's 
				 * room to start another
				 * word.
				 */

				width = XmbTextEscapement(fontset,
						(const char *)str2,i);

				if (width >= win) {
				/*
				 * Break the line if we can't start
				 * another word.
				 */
					*str1 = '\n';
					gotone = TRUE;
				}
				/*
				 * Save the pointer to
				 * space and Step past the space
				 */
				save = str1;
				str1++;
				break;
				case '\n':
				/*
				 * Forced break.  Step past the \n.
				 * Note the fall through to default.
				 */
					str1++;
				default:
				/*
				 * End of string.
				 */
					gotone = TRUE;
				break;
				} /* switch */

				} else if (width > win) {
			/*
			 * We know that we have something
			 */
					gotone = TRUE;

			/*
			 * See if there is at least one 
			 * space to back up for.
			 */
					if (wordindex) {
						str1 = save;
						*str1++ = '\n';
					} else
				/*
				 * We have a single word which is too long
				 * for the available window.  Let the text
				 * clip rectangle handle it.
				 */
					if (*str1)
						*str1++ = '\n';
				} else /* (width == win) */ {
					switch (*str1) {
					case ' ':
					/*
					 * If we stopped on a space, 
					 * change it.
					 */
						*str1 = '\n';
					case '\n':
						/*
						 * Step past the \n.
						 */
						str1++;
					default:
						gotone = TRUE;
					break;
					} /* switch */
                        	} /* else */
		} /* while !gotone */
	} /* while *str1 */
	break;
   case OL_WC_STR_REP:
	fontset = (XFontSet)stw->primitive.font;
	wstr1 = (wchar_t *)stp->output_string;
	while (*wstr1) {

		i = 0;
		width = 0;
		wordindex = -1;
		gotone = FALSE;
		wstr2 = wstr1;

		while (!gotone) {
			if ((stp->strip) && (wordindex == -1))
				while (*wstr1 == L' ')
					wstr1++;
			/*
			 *Step through until a character that we can
			 * break on.
			 */
			while ((*wstr1 != L' ') && 
			      (*wstr1 != L'\n') && 
					(*wstr1))
				i++, wstr1++;

			wordindex++;

			width = (i ? 
				XwcTextEscapement(fontset, wstr2,i) : 0);

			if (width < win) {

			/*
			 * If the current string fragment 
			 * is shorter than the 
			 * available window.  Check to see if we are at a 
			 * forced break or not, and process accordingly.
			 */

			switch (*wstr1) {
				case L' ':
				/*
				 * Add the space to the char count 
				 */
					i++;
				/*
				 * Check to see there's 
				 * room to start another
				 * word.
				 */

				width = 
				     XwcTextEscapement(fontset, wstr2,i);

				if (width >= win) {
				/*
				 * Break the line if we can't start
				 * another word.
				 */
					*wstr1 = L'\n';
					gotone = TRUE;
				}
				/*
				 * Step past the space
				 */
				wstr1++;
				break;
				case L'\n':
				/*
				 * Forced break.  Step pase the \n.
				 * Note the fall through to default.
				 */
					wstr1++;
				default:
				/*
				 * End of string.
				 */
					gotone = TRUE;
				break;
				} /* switch */

				} else if (width > win) {
			/*
			 * We know that we have something
			 */
					gotone = TRUE;

			/*
			 * See if there is at least one 
			 * space to back up for.
			 */
					if (wordindex) {
						wstr1--;
						while (*wstr1 != L' ')
							wstr1--;
						*wstr1++ = L'\n';
					} else
				/*
				 * We have a single word which is too long
				 * for the available window.  Let the text
				 * clip rectangle handle it.
				 */
					if (*wstr1)
						*wstr1++ = L'\n';
				} else /* (width == win) */ {
					switch (*wstr1) {
					case L' ':
					/*
					 * If we stopped on a space, 
					 * change it.
					 */
						*wstr1 = L'\n';
					case L'\n':
						/*
						 * Step past the \n.
						 */
						wstr1++;
					default:
						gotone = TRUE;
					break;
					} /* switch */
                        	} /* else */
		} /* while !gotone */
	} /* while *wstr1 */
	break;

} /* switch */

} /* end of Format Text */

/*************************************<->*************************************
 *
 *	static void GetTextRect(newstw)
 *		StaticTextWidget	newstw;
 *  
 *	Description:
 *	-----------
 *		Sets newstw->static_text.TextRect to 
 *		appropriate values given 
 *		newstw->core.width and newstw->static_text.input_string.  
 *		The
 *		string is formatted if necessary.
 *		
 *		A newstw->core.width value of 0 tells this 
 *		procedure to choose 
 *		its own size based soley on newstw->static_text.
 *		input_string.  
 *
 *	Inputs:
 *	------
 *
 *	Outputs:
 *	-------
 *
 *	Procedures Called
 *	-----------------
 *		Maxline
 *		RetCount
 *
 *************************************<->***********************************/
static void 
GetTextRect(StaticTextWidget newstw)
{
	int		numrets;
	Dimension	fheight, maxwin, maxlen;
	StaticTextPart	*stp;
	XFontStruct 	*font;
	XFontSet	fontset;
	XFontSetExtents *fset_extents;

	stp = &(newstw->static_text);
	/*
	 * Set the line height from the font structure.
	 */
	if(newstw->primitive.text_format == OL_SB_STR_REP){
		font = (XFontStruct *)(newstw->primitive.font);
		fheight = (Dimension) (font->ascent +font->descent);
	}
	else if(newstw->primitive.text_format == OL_WC_STR_REP){
		fontset = (XFontSet)(newstw->primitive.font);
		fset_extents = XExtentsOfFontSet(fontset);
		fheight = fset_extents->max_logical_extent.height;
	} else{
		fontset = (XFontSet)(newstw->primitive.font);
		fset_extents = XExtentsOfFontSet(fontset);
		fheight = fset_extents->max_logical_extent.height;
	}


	/* Do the stripping stuff, if reqd before anything else */
		if (stp->strip) StripSpaces(newstw);

	if (!stp->recompute_size && newstw->core.width <= 0)
	    newstw->core.width = 1;
	    
	if (newstw->core.width)
	{
		/*
		 * We were requested with a specific width.  We must
		 * fit ourselves to it.
		 *
		 * The maximum available window width is...
		 */
		maxwin = newstw->core.width - (2 * stp->internal_width)
			- (2 * newstw->static_text.highlight_thickness);
		if (stp->wrap)
                {
			if ((maxlen = Maxline(newstw)) <= 
				maxwin)
				/*
				 * We fit without formatting.
				 */
				stp->TextRect.width = maxlen;
			else
			{
				stp->TextRect.width = maxwin; 
				/* 
				 * Make the string fit.
				 */
				FormatText(newstw);
			}
                }
		else
			stp->TextRect.width = Maxline(newstw);
	}
	else
		stp->TextRect.width = Maxline(newstw);
	/*
	 * See how tall the string wants to be.
	 */
	numrets = RetCount(newstw->primitive.text_format,
					stp->output_string);
		/* grep SC for comments */
	stp->TextRect.height = (Dimension) (int)(numrets*
				((stp->line_space/100.0+1)*fheight));

		/* SC:cast it to (int) because 1st part may be negative */
	if ((newstw->core.height) &&
		((int)(newstw->core.height - (2 * stp->internal_height)
			- (2 * newstw->static_text.highlight_thickness)) < 
			(int)stp->TextRect.height))
			/*
			 * Shorten the TextRect if the string wants
			 * to be too tall.
			 */
			stp->TextRect.height = newstw->core.height
				- (2 * stp->internal_height)
				- (2 * newstw->static_text.highlight_thickness);
}

/*************************************<->*************************************
 *
 *	static void SetSize(newstw)
 *		StaticTextWidget	newstw;
 *
 *	Description:
 *	-----------
 *		Copies newstw->static_text.input_string into output_string
 *
 *		Sets newstw->core.width, newstw->core.height, and
 *		newstw->static_text.TextRect appropriately, formatting the 
 *		string if necessary.
 *
 *		The Clip Rectangle is placed in the window.
 *
 *	Inputs:
 *	------
 *		stw = A meaningful StaticTextWidget.
 *
 *	Outputs:
 *	-------
 *
 *	Procedures Called
 *	-----------------
 *		strcpy
 *		GetTextRect
 *
 *************************************<->***********************************/
static void 
SetSize(StaticTextWidget newstw)
{
	StaticTextPart	*stp;
	XFontStruct *font;
	XFontSet   fontset;
	XFontSetExtents *fset_extents;

	stp = &(newstw->static_text);
	/*
	 * Copy the input string into the output string.
	 */

  switch(newstw->primitive.text_format) {
    case OL_SB_STR_REP:
	font = (XFontStruct *)(newstw->primitive.font);
	if (stp->output_string != (OlStr)NULL)
		XtFree(stp->output_string);
	stp->output_string = (OlStr)XtMalloc(
				strlen((char *)stp->input_string)+1);
	strcpy((char *)stp->output_string,(char *)stp->input_string);
	if (*((char *)stp->output_string))
		/*
		 * If we have a string then size it.
		 */
		GetTextRect(newstw);
	else {
		stp->TextRect.width = 0;
		stp->TextRect.height = (Dimension) (font->ascent +
				       			font->descent);
	}
	break;
    case OL_MB_STR_REP:
	fontset = (XFontSet)(newstw->primitive.font);
	if (stp->output_string != (OlStr)NULL)
		XtFree(stp->output_string);
	stp->output_string = (OlStr)XtMalloc(
				strlen((char *)stp->input_string) +1);
	strcpy((char *)stp->output_string,(char *)stp->input_string);
	if (*((char *)stp->output_string))
		/*
		 * If we have a string then size it.
		 */
		GetTextRect(newstw);
	else {
		stp->TextRect.width = 0;
		fset_extents = XExtentsOfFontSet(fontset);
		stp->TextRect.height = fset_extents->
						max_logical_extent.height;
	}
	break;
    case OL_WC_STR_REP:
	fontset = (XFontSet)(newstw->primitive.font);
	if (stp->output_string != (OlStr)NULL)
		XtFree(stp->output_string);
	stp->output_string = (OlStr)XtMalloc(
				(wslen((wchar_t *)(stp->input_string))
						+ 1)*wcsiz);
	wscpy((wchar_t *)stp->output_string,(wchar_t *)stp->input_string);
	if (*((wchar_t *)stp->output_string))
		/*
		 * If we have a string then size it.
		 */
		GetTextRect(newstw);
	else {
		stp->TextRect.width = 0;
		fset_extents = XExtentsOfFontSet(fontset);
		stp->TextRect.height = fset_extents->
						max_logical_extent.height;
	}
	break;

  } /* switch */

	/*
	 * Has a width been specified?
	 */
	if (newstw->core.width)
        {
		if ((int)(newstw->core.width	/* grep SC for comments */
			- (2 * newstw->static_text.highlight_thickness)
			- (2 * stp->internal_width)) > (int)stp->TextRect.width)
                {
			/*
			 * Use the extra space according to the gravity
			 * resource setting.
			 */
			switch (stp->gravity)
			{
				case EastGravity:
				case NorthEastGravity:
				case SouthEastGravity:
					stp->TextRect.x = (Position) (newstw->core.width -
						(newstw->static_text.highlight_thickness + 
						stp->TextRect.width + stp->internal_width));
				break;
				case WestGravity:
				case NorthWestGravity:
				case SouthWestGravity:
					stp->TextRect.x = (Position)(stp->internal_width + 
						newstw->static_text.highlight_thickness);
				break;
				default: /* grep SC for comments */
					stp->TextRect.x = (Position) ((int)(newstw->core.width
						- stp->TextRect.width) / 2);
				break;
			}
                }
		else
			/*
		 	* We go to the left.
		 	*/
			stp->TextRect.x = (Position) (newstw->static_text.highlight_thickness + stp->internal_width);
        }
	else
	{
		/*
		 * We go to the left.
		 */
		stp->TextRect.x = (Position) (newstw->static_text.highlight_thickness + stp->internal_width);
		newstw->core.width = stp->TextRect.width
			+ (2 * stp->internal_width)
			+ (2 * newstw->static_text.highlight_thickness);
	}
	/*
	 * Has a height been specified?
	 */
	if (newstw->core.height)
        {
		if ((int)(newstw->core.height - (2 * stp->internal_height)
			- (2 * newstw->static_text.highlight_thickness)) > 
			(int)stp->TextRect.height) /* grep SC for comments */
                {
			/*
			 * Use the extra space according to the gravity
			 * resource setting.
			 */
			switch (stp->gravity)
			{
				case NorthGravity:
				case NorthEastGravity:
				case NorthWestGravity:
					stp->TextRect.y = (Position) (stp->internal_height +
						newstw->static_text.highlight_thickness);
				break;
				case SouthGravity:
				case SouthEastGravity:
				case SouthWestGravity:
					stp->TextRect.y = (Position) (newstw->core.height -
						(newstw->static_text.highlight_thickness + 
						stp->TextRect.height + stp->internal_width));
				break;
				default: /* grep SC for comments */
					stp->TextRect.y = (Position) ((int)(newstw->core.height 
						- stp->TextRect.height)/ 2);
				break;
			}
                }
		else
                {
			/*
			 * We go to the top.
			 */
			stp->TextRect.y = (Position) (newstw->static_text.highlight_thickness + stp->internal_height);
                }
        }
	else
	{
		/*
		 * We go to the top.
		 */
		stp->TextRect.y = (Position) (newstw->static_text.highlight_thickness
			+ stp->internal_height);
		/*
		 * We add our size to the current size.
		 * (Primitive has already added highlight_thicknesses.)
		 */
		newstw->core.height = stp->TextRect.height
			+ (2 * stp->internal_height)
			+ (2 * newstw->static_text.highlight_thickness);
	}
/*
**  Keep intrinsics happy by enforcing a minimum size.
*/
	if (newstw->core.width <= 0) {
		newstw->core.width = 1;
	}

	if (newstw->core.height <= 0) {
		 XFontSetExtents *fset_extents;

		if(newstw->primitive.text_format == OL_SB_STR_REP){
			newstw->core.height = 
			(Dimension) (font->ascent + font->descent);
		}
		else if(newstw->primitive.text_format == OL_WC_STR_REP){
			fset_extents = 
			XExtentsOfFontSet(fontset);
			newstw->core.height = 
				fset_extents->max_logical_extent.height;
		}
		else{
			fset_extents = 
			XExtentsOfFontSet(fontset);
			newstw->core.height = 
				fset_extents->max_logical_extent.height;
		}
	}

	if (newstw->core.height <= 0)
		newstw->core.height = 1;

}

static void 
ProcessBackslashes(OlStrRep tf,OlStr output, OlStr input)
{
unsigned char *out, *in;
wchar_t  *wout, *win;
int len;

switch(tf) {
  case OL_SB_STR_REP:
	out = (unsigned char *)output;
	in = (unsigned char *)input;
	while(*in)
		if (*in == '\\')
		{
			in++;
			switch (*in)
			{
				case 'n':
					*out++ = '\n';
				break;
				case 't':
					*out++ = '\t';
				break;
				case 'b':
					*out++ = '\b';
				break;
				case 'r':
					*out++ = '\r';
				break;
				case 'f':
					*out++ = '\f';
				break;
				default:
					*out++ = '\\';
					*out++ = *in;
				break;
			}
			in++;
		}
		else
			*out++ = *in++;
	*out = '\0';
	break;
  case OL_MB_STR_REP:
	out = (unsigned char *)output;
	in = (unsigned char *)input;
	while((len = mblen((char *)in, MB_CUR_MAX)) > 0)
		if (len ==1 && *in == '\\') {
			in++;
			len = mblen((char *)in,MB_CUR_MAX);
			if(len == 1) 
			switch (*in) {
				case 'n':
					*out++ = '\n';
				break;
				case 't':
					*out++ = '\t';
				break;
				case 'b':
					*out++ = '\b';
				break;
				case 'r':
					*out++ = '\r';
				break;
				case 'f':
					*out++ = '\f';
				break;
				default:
					*out++ = '\\';
					*out++ = *in;
				break;
			} 
			else {
				*out++ = '\\';
				memmove((XtPointer)out, 
						(XtPointer)in, len);
			}
			in+=len;
		} else {
			memmove((XtPointer)out, (XtPointer)in, len);
			out += len;
			in += len;
		}
	*out = '\0';
	break;
  case OL_WC_STR_REP:
	wout = (wchar_t *)output;
	win = (wchar_t *)input;
	while(*win)
		if (*win == L'\\')
		{
			win++;
			switch (*win)
			{
				case L'n':
					*wout++ = L'\n';
				break;
				case L't':
					*wout++ = L'\t';
				break;
				case L'b':
					*wout++ = L'\b';
				break;
				case L'r':
					*wout++ = L'\r';
				break;
				case L'f':
					*wout++ = L'\f';
				break;
				default:
					*wout++ = L'\\';
					*wout++ = *win;
				break;
			}
			win++;
		}
		else
			*wout++ = *win++;
	*wout = wcnull;
	break;
   }
}
/*************************************<->*************************************
 *
 *  static void Initialize (request, new, args, p_num_args)
 *  	Widget request, new;
 *		ArgList args;
 *		Cardinal *p_num_args;
 *  
 *	Description:
 *	-----------
 *		See XToolKit Documentation
 *
 *
 *	Inputs:
 *	------
 *
 *	Outputs:
 *	-------
 *
 *	Procedures Called
 *	-----------------
 *		Xmalloc
 *		ProcessBackslashes
 *		SetSize
 *
 *************************************<->***********************************/
/* ARGSUSED */
static void 
Initialize (StaticTextWidget req, StaticTextWidget new, ArgList args, Cardinal *p_num_args)
{
	OlStr s;

	new->static_text.output_string = NULL;

	if (new->static_text.input_string != (OlStr)NULL)  {
		/*
		 * Copy the input string into local space.
		 */
		if(new->primitive.text_format == OL_WC_STR_REP)
		s = (OlStr)XtMalloc((wslen((wchar_t *)
			(new->static_text.input_string))+ 1)*
						wcsiz);
		else
		s = (OlStr)XtMalloc(strlen((char *)
					new->static_text.input_string)+1);
		ProcessBackslashes(new->primitive.text_format,
					s,
					new->static_text.input_string);
		new->static_text.input_string = s;
	}
	else  {
		if(new->primitive.text_format == OL_WC_STR_REP){
			s = (OlStr)XtMalloc(wcsiz);
			*((wchar_t *)s) = wcnull;
		}
		else {
			s = (OlStr)XtMalloc(1);
			*((unsigned char *)s) = '\0';
		}
			
		new->static_text.input_string = s;
	}

	ValidateInputs(new);
	new->core.width = req->core.width;
	new->core.height = req->core.height;

 	new->static_text.line_table = (unsigned char **) 
			XtMalloc((Cardinal)(5 * sizeof(char *)));
	new->static_text.line_lens = (int *)
			XtCalloc((Cardinal)5,(Cardinal)sizeof(int));
 	new->static_text.line_count = 5;

 	new->static_text.selection_mode = 0;
 	new->static_text.save_mode = 0;
 	new->static_text.selection_start= NULL;
 	new->static_text.selection_end=  NULL;
 	new->static_text.oldsel_start=  NULL;
 	new->static_text.oldsel_end=  NULL;
  	new->static_text.clip_contents=  (OlStr)NULL;
 	new->static_text.ev_x = 0;
 	new->static_text.ev_y = 0;
 	new->static_text.old_x = 0;
 	new->static_text.old_y = 0;
 	new->static_text.highlight_thickness = 0;

	/* We basically need a stipple to draw inactive text. The
	 * stipple needs to be created for the current "screen" and
	 * with the "current visual". _OlgxGetDeviceData() maintains
	 * a global linked list of device_recs corresponding to each 
	 * screen +  visual + scale combination. We just borrow one
	 * from that list. Note that this avoids creation of a pixmap
	 * for each instance of the statictext -JMK
	 */
	new->static_text.devData = _OlgxGetDeviceData((Widget)new,
	 				OL_DEFAULT_POINT_SIZE);
        new->static_text.normal_GC = (GC)NULL;
        new->static_text.hilite_GC = (GC)NULL;

	text_format = new->primitive.text_format;

	SetSize(new);
}

/*************************************<->*************************************
 *
 *	static Boolean SetValues(current, request, new, last)
 *		StaticTextWidget current, request, new;
 *		Boolean last;
 *
 *	Description:
 *	-----------
 *		See XToolKit Documentation
 *
 *
 *	Inputs:
 *	------
 *
 *	Outputs:
 *	-------
 *
 *	Procedures Called
 *	-----------------
 *		ValidateInputs
 *		strcmp
 *		SetSize
 *		SetUpGCs
 *
 *************************************<->***********************************/
/* ARGSUSED */
static Boolean 
SetValues(StaticTextWidget current, StaticTextWidget request, StaticTextWidget new, ArgList args, Cardinal *num_args)
{
	Boolean	flag = FALSE;
	Boolean	newstring = FALSE;
	OlStr	newstr, curstr;
	OlStr	s;
	Dimension	new_w, new_h;
	StaticTextPart *	newstp = &(new->static_text);
	StaticTextPart *	curstp = &(current->static_text);
	Screen			*scr = XtScreenOfObject(current);
	Dimension		screen_width = WidthOfScreen(scr);

	if (newstp->input_string != curstp->input_string) {
		newstring = TRUE;
		/*
		 * Copy the input string into local space.
		 */
		if (new->static_text.input_string != (OlStr)NULL)  {
		/*
		 * Copy the input string into local space.
		 */
		if(new->primitive.text_format == OL_WC_STR_REP)
		s = (OlStr)XtMalloc((wslen((wchar_t *)
			(new->static_text.input_string))+ 1)
						*wcsiz);
		else
		s = (OlStr)XtMalloc(strlen((char *)
					new->static_text.input_string)+1);
		ProcessBackslashes(new->primitive.text_format,
					s,
					new->static_text.input_string);
		} else  {
			if(new->primitive.text_format == OL_WC_STR_REP){
				s = (OlStr)XtMalloc(wcsiz);
				*((wchar_t *)s) = wcnull;
			} else {
				s = (OlStr)XtMalloc(1);
				*((unsigned char *)s) = '\0';
		        }
			
		}

		/*
		 * Deallocate the old string.
		 */
		XtFree(curstp->input_string);
		/*
		 * Have everybody point to the new string.
		 */
		newstp->input_string = s;
		curstp->input_string = s;
		request->static_text.input_string = s;

		/*
		 * clear the selection
		 */
 		newstp->selection_mode = 0;
 		newstp->selection_start=  NULL;
 		newstp->selection_end=  NULL;
 		newstp->oldsel_start=  NULL;
 		newstp->oldsel_end=  NULL;
	}

	ValidateInputs(new);

#define	DIFFERENT(field)	(newstp->field != curstp->field)

	if (DIFFERENT(selectable) && !newstp->selectable) {

		/* Clear any existing selections */
		newstp->selection_mode = 0;
		newstp->selection_start=  NULL;
		newstp->selection_end=  NULL;
		newstp->oldsel_start=  NULL;
		newstp->oldsel_end=  NULL;

		/* Need to redisplay too .. */
		flag = True;
	}

	if ((newstring) ||
		DIFFERENT(strip) ||
		DIFFERENT(wrap) ||
		DIFFERENT(recompute_size) ||
		DIFFERENT(line_space) ||
		DIFFERENT(alignment) ||
		DIFFERENT(gravity) ||
		DIFFERENT(internal_height) ||
		DIFFERENT(internal_width) ||
		(request->primitive.font != current->primitive.font) ||
		((new->core.width <= 0) || (new->core.height <= 0)) ||
		(request->core.width != current->core.width) ||
		(request->core.height != current->core.height))
	{

		if (new->core.width <= 0) {
			OlWarning(dgettext(OlMsgsDomain,
				"StaticTextWidget: Cannot resize to a non-positive width."));
			new->core.width = 1;
		}
		if (new->core.height <= 0) {
			OlWarning(dgettext(OlMsgsDomain,
				"StaticTextWidget: Cannot resize to a non-positive height."));
			new->core.height = 1;
		}
#undef DIFFERENT
/*
**	If we're allowed to recompute our size, set height/width to zero
**	to indicate this.
*/
		if (newstp->recompute_size) {
			new->core.width = 0;
			new->core.height = 0;
		}

		/*
		 * Call SetSize to get the new size.
		 */
		SetSize(new);

		/*
		 * wrap text if it is longer than screen and it is wrappable
		 */
		if (new->core.width > screen_width && newstp->wrap)
		{
		    new->core.width = screen_width - 100;
		    new->core.height = 0;
		    SetSize(new);
		}

		/*
		 * Save changes that SetSize does to the layout.
		 */
		new_w = new->core.width;
		new_h = new->core.height;
		/*
		 * In case our parent won't let us change size we must
		 * now restore the widget to the current size.
		 */
		new->core.width = current->core.width;
		new->core.height = current->core.height;
		SetSize(new);
		/*
		 * Reload new with the new sizes in order cause XtSetValues
		 * to invoke our parent's geometry management procedure.
		 */
		new->core.width = new_w;
		new->core.height = new_h;

		flag = TRUE;
	}

	if ((new->primitive.font_color != current->primitive.font_color) ||
	    (new->core.background_pixel != current->core.background_pixel) ||
	    (new->primitive.font != current->primitive.font) ||
	    (XtIsSensitive((Widget)new) != XtIsSensitive((Widget)current))) {
		if (XtIsRealized((Widget)new)) {
			SetUpGCs(new,XtIsSensitive((Widget)new));
		}
		flag = TRUE;
	}
	/* JMK - Rebuild the linetable, if "string" has been changed.
		Earlier, the Redisplay routine used to take care of
		this,but with users doin crazy things, like changing
		the text for a KeyPress event, can't delay this !
	*/
	if (newstring)
		StBuildLineTable(new);
	
	return(flag);
}

/*************************************<->*************************************
 *
 *	static void Realize(w , valueMask, attributes)
 *		Widget              	w;
 *		XtValueMask         	*valueMask;
 *		XSetWindowAttributes	*attributes;
 *
 *	Description:
 *	-----------
 *		See XToolKit Documentation
 *
 *	Inputs:
 *	------
 *
 *	Outputs:
 *	-------
 *
 *	Procedures Called
 *	-----------------
 *		XtCreateWindow
 *		SetUpGCs
 *
 *************************************<->***********************************/
static void 
Realize(StaticTextWidget stw, XtValueMask *valueMask, XSetWindowAttributes *attributes)
{

	XtCreateWindow((Widget)stw,InputOutput,(Visual *) CopyFromParent,
			*valueMask,attributes);
	SetUpGCs(stw,XtIsSensitive((Widget)stw));
}

/*************************************<->*************************************
 *
 *	static void Destroy(stw)
 *		StaticTextWidget	stw;
 *
 *	Description:
 *	-----------
 *		See XToolKit Documentation
 *
 *	Inputs:
 *	------
 *
 *	Outputs:
 *	-------
 *
 *	Procedures Called
 *	-----------------
 *		XtFree
 *		XtReleaseGC
 *
 *************************************<->***********************************/
static void 
Destroy(StaticTextWidget stw)
{
	XtFree(stw->static_text.input_string);
	XtFree(stw->static_text.output_string);
	XtFree((char *)(stw->static_text.line_table));
	XtFree((char *)(stw->static_text.line_lens));
	if (XtIsRealized((Widget)stw)) {
		XtReleaseGC((Widget)stw,stw->static_text.normal_GC);
		XtReleaseGC((Widget)stw,stw->static_text.hilite_GC);
	}
        _OlgxFreeDeviceDataRef(stw->static_text.devData);

}


/*****************************************************************************
 *
 *	static void Redisplay(stw)
 *		StaticTextWidget	stw;
 *
 ****************************************************************************/
static void 
Redisplay(StaticTextWidget stw)
{
	XEvent			junk_event;
	int			hpad,
				vpad;
	unsigned char*		cstring;

	/* Compress multiple exposes */
	while (XCheckWindowEvent(XtDisplay(stw), XtWindow(stw), ExposureMask,
			&junk_event))
		/*EMPTYBODY*/;

	StBuildLineTable(stw);

	/* If widget is insensitive, do not highlight text */
	cstring = (unsigned char*)(stw->static_text.output_string);

	if (XtIsSensitive((Widget)stw)) {

		if (stw->primitive.text_format == OL_WC_STR_REP) {
		    StDisplaySubstring(stw, 
			(unsigned char *)stw->static_text.output_string,
			(unsigned char *)((char*)stw->static_text.output_string +
			   sizeof (wchar_t) * wslen( (const wchar_t*)cstring)),
				stw->static_text.normal_GC, TRUE);
		} else {
		    StDisplaySubstring(stw, 
			(unsigned char *)stw->static_text.output_string,
			(unsigned char *)((char*)stw->static_text.output_string +
					strlen((const char*)cstring)),
				stw->static_text.normal_GC, TRUE);
		}
		StHighlightSelection(stw, True);

	} else {

		if (stw->primitive.text_format == OL_WC_STR_REP)
		    StDisplaySubstring(stw, 
			(unsigned char *)stw->static_text.output_string,
			(unsigned char *)((char*)stw->static_text.output_string +
			sizeof (wchar_t) * wslen((const wchar_t*)cstring)),
				stw->static_text.normal_GC, FALSE);
		else
		    StDisplaySubstring(stw, 
			(unsigned char *)stw->static_text.output_string,
			(unsigned char *)((char*)stw->static_text.output_string +
				strlen((const char*)cstring)),
				stw->static_text.normal_GC, FALSE);
	}

	/* Draw the "clipping" rectangles along the south and east edges */
	hpad = vpad = stw->static_text.highlight_thickness;
	hpad += stw->static_text.internal_width;
	vpad += stw->static_text.internal_height;

	if (hpad > 0)
	    XClearArea(XtDisplay(stw), XtWindow(stw),
		stw->core.width - hpad, vpad,
		hpad, stw->core.height - vpad, False);
	if (vpad > 0)
	    XClearArea(XtDisplay(stw), XtWindow(stw),
		hpad, stw->core.height - vpad,
		stw->core.width - hpad - hpad, vpad, False);
}
/*************************************<->*************************************
 *
 *	static void Resize(stw)
 *		StaticTextWidget	stw;
 *
 *	Description:
 *	-----------
 *		See XToolKit Documentation
 *
 *	Inputs:
 *	------
 *
 *	Outputs:
 *	-------
 *
 *	Procedures Called
 *	-----------------
 *		SetSize
 *
 *************************************<->***********************************/
static void 
Resize(StaticTextWidget stw)
{
int ostart, oend, cstart, cend;
unsigned char *base;
Boolean sel = False;
StaticTextPart *stp = &(stw->static_text);

	if (stw->core.width <= 0)
	{
		OlWarning(dgettext(OlMsgsDomain,
			"StaticTextWidget: Cannot resize to a non-positive width."));
		stw->core.width = 1;
	}
	if (stw->core.height <= 0)
	{
		OlWarning(dgettext(OlMsgsDomain,
			"StaticTextWidget: Cannot resize to a non-positive height."));
		stw->core.height = 1;
	}

	/*
	 * Same as at initialization except just look at the new widget.
	 */
	if(stp->selection_start ||
		stp->oldsel_start) {
		base = (unsigned char *)stp->output_string;
		ostart = stp->oldsel_start - base;
		oend = stp->oldsel_end - base; 
		cstart = stp->selection_start - base;
		cend = stp->selection_end - base; 
		sel = True;
		stp->save_mode = stp->selection_mode;
		stp->selection_mode = NUM_SELECTION_TYPES + 1;
	}
	SetSize(stw);
	StBuildLineTable(stw);
	if(sel) {
		base = (unsigned char *)stp->output_string;
		stp->selection_start = base + cstart;
		stp->selection_end = base + cend;
		stp->oldsel_end = base + oend;
		stp->oldsel_start = base + ostart;
	}
}

/*
** Selection/Clipboard routines
*/

static void 
SelectStart(StaticTextWidget stw, XEvent *event)
{
	TakeFocus((Widget) stw, event);
	StStartAction(stw, event);
	StAlterSelection(stw, StaticTextSelect);
	StEndAction(stw);
}

static void	
LosePrimary(Widget w, Atom *atom)
{
        StaticTextWidget stw = (StaticTextWidget)w;
/*
** We make the start of the selection equal to the end of the selection,
** unhighlighting the text.
*/
	stw->static_text.selection_start = stw->static_text.selection_end;
 	stw->static_text.selection_mode = 0;
	StHighlightSelection(stw,True);
}


static void	
LoseClipboard(Widget w, Atom *atom)
{
        StaticTextWidget stw = (StaticTextWidget)w;
	if ( stw->static_text.clip_contents != (OlStr)NULL) {
		XtFree ((char *)stw->static_text.clip_contents);
		stw->static_text.clip_contents = 0;
	}
}

static Boolean	
ConvertSelection(Widget w, Atom *selection, Atom *target, Atom *type_return, XtPointer *value_return, long unsigned int *length_return, int *format_return)
{
        StaticTextWidget stw = (StaticTextWidget)w;
	int	i;
	char	*start, *end, *buffer;
	Atom	atom;
	Atom	*atoms;
	OlStr	str;
	wchar_t wcnull = wcnull;
	OlStrRep	rep = stw->primitive.text_format;

if (*selection != XA_PRIMARY && *selection 
			!=  XA_CLIPBOARD(XtDisplay(stw))) {
		OlWarning(dgettext(OlMsgsDomain,
			"stext:	Request for unknown selection ignored\n"));
		return (False);
	}

if (*target == (atom = OlInternAtom(XtDisplay(stw), _OL_COPY_NAME))) {
		WriteToCB(stw);
		*value_return = NULL;
		*length_return = NULL;
		*format_return = NULL;
		*type_return = atom;
		return (False);
	}

else if (*target == (atom = OlInternAtom(XtDisplay(stw), _OL_CUT_NAME))) {
		OlWarning(dgettext(OlMsgsDomain,
			"stext:	Not allowed to cut from static text\n"));
		return (False);
	}

else if (*target == OlInternAtom(XtDisplay(stw), TARGETS_NAME)) {
      		*format_return = (int)(8*sizeof(Atom));
      		*length_return = (unsigned long)4;
      		atoms=(Atom *)
			XtMalloc((unsigned)((*length_return)*(sizeof(Atom))));
      		atoms[0] = OlInternAtom(XtDisplay(stw), TARGETS_NAME);
      		atoms[1] = XA_STRING;
      		atoms[2] = OlInternAtom (XtDisplay(stw), LENGTH_NAME);
      		atoms[3] = OlInternAtom (XtDisplay(stw), COMPOUND_TEXT_NAME);
      		*value_return = (XtPointer)atoms;
      		*type_return = XA_ATOM;
      		return (True);
      }

else if (*selection == XA_PRIMARY && (*target == XA_STRING ||
	*target == OlInternAtom(XtDisplay(stw),COMPOUND_TEXT_NAME))) {
		StFetchSelection(stw, (unsigned char **)&start,
				 (unsigned char **)&end);
		i = end - start; /* i is offset in bytes */

        if(*target == XA_STRING)  {
		switch(rep) {
			case OL_WC_STR_REP:
				{
				int num_wchars = (int)(i/wcsiz);

				buffer = XtMalloc((1 + num_wchars)*MB_CUR_MAX);
				i = (int)wcstombs(buffer,(wchar_t *)start,num_wchars);
				if(i ==  (size_t)-1)
					OlError(dgettext(OlMsgsDomain,
					"stext: Fatal error in wcstombs\n"));
				buffer[i] = '\0';
				break;
				}
			default:
				buffer = XtMalloc(1 + i);
				memmove((XtPointer)buffer,(XtPointer)start,i);
				buffer[i] = '\0';
				break;
		}
		
		*format_return = 8;
                *length_return = i + 1; /* include null byte */
                *value_return  = (XtPointer) buffer;
                *type_return   = XA_STRING; 

		return(True);

	} else { 

		/* COMPOUND_TEXT */
		char *ctbuf;
		int ctlen;
		char *clip_cont;

		clip_cont = XtMalloc(i+1);
		memmove((XtPointer)clip_cont,start,i);
		clip_cont[i] = '\0';
		ctbuf          = str_methods[rep].StrToCT(XtDisplay(w),
                                                        clip_cont, &ctlen);
            	buffer         = XtMalloc(ctlen + 1);
            	memcpy(buffer, ctbuf, ctlen);
            	buffer[ctlen]  = '\0';
		XtFree(clip_cont);

            	*format_return = 8;
            	*length_return = ctlen + 1; /* include null byte */
            	*value_return  = (XtPointer) buffer;
            	*type_return   = OlInternAtom(XtDisplay(w), COMPOUND_TEXT_NAME);

		return (True);
	} 

      }

else if ((*target == XA_STRING || *target ==  
			OlInternAtom(XtDisplay(w), COMPOUND_TEXT_NAME)) && 
				 *selection == XA_CLIPBOARD(XtDisplay(stw))) {
     if(*target == XA_STRING) {
	switch(rep)  {
		case OL_WC_STR_REP:
			{
			int num_wchars = 
				(int)wslen((wchar_t *)
					stw->static_text.clip_contents);

			buffer =  (char *)XtMalloc((1+num_wchars)*MB_CUR_MAX);
			i = wcstombs((char *)buffer,
				(wchar_t *)stw->static_text.clip_contents,
								num_wchars);
			if(i ==  (size_t)-1)
				OlError(dgettext(OlMsgsDomain,
					"stext: Fatal error in wcstombs\n"));
			}
			break;
		default:
			buffer = XtNewString((char *)
					stw->static_text.clip_contents);
			break;
	}
		*value_return = buffer;
		*length_return = strlen((const char *)buffer) + 1;
		*format_return = 8;
		*type_return = XA_STRING;
		return (True);
	} else {
		char *ctbuf;
		int ctlen;
		char *clip_cont = stw->static_text.clip_contents;

		ctbuf          = str_methods[rep].StrToCT(XtDisplay(w),
                                                        clip_cont, &ctlen);
            	buffer         = XtMalloc(ctlen + 1);
            	memcpy(buffer, ctbuf, ctlen);
            	buffer[ctlen]  = '\0';

            	*format_return = 8;
            	*length_return = ctlen + 1; /* include null byte */
            	*value_return  = (XtPointer) buffer;
            	*type_return   = OlInternAtom(XtDisplay(w),
						COMPOUND_TEXT_NAME);

		return (True);
     } 
}

else if (*target == OlInternAtom(XtDisplay(stw), LENGTH_NAME)) {

		int *intbuffer;

       		intbuffer = (int *) XtMalloc(sizeof(int));
		switch(rep) {
		case OL_WC_STR_REP:
			{
			wchar_t  *temp = (wchar_t *)
					stw->static_text.clip_contents;
			char *buffer = XtMalloc((wslen(temp) +1)*MB_CUR_MAX);
			*intbuffer = (int)wcstombs(buffer,temp,wslen(temp)); 
			XtFree((char *)buffer);
			}
			break;
		default:
       			*intbuffer = (int) (strlen ((char *)
					stw->static_text.clip_contents));
			break;
		}

       	 	*value_return = (XtPointer)intbuffer;
        	*length_return = 1;
        	*format_return = sizeof(int) * 8;
        	*type_return = (Atom) *target;
		return (True);
      }

else {

           	char        *atom;
           	static char prefix[] = "_SUN_SELN";
 
	        atom = XGetAtomName(XtDisplay(stw), *target);
 
                if (strncmp(prefix, atom, strlen(prefix)) != 0)
		      OlWarning(dgettext(OlMsgsDomain,
     		      		"StaticText: Can't convert"));
           	XFree(atom);
	   	return (False);
   } /* else */     

} /* ConvertSelection */


static void 
SelectAdjust(StaticTextWidget stw, XEvent *event)
{
	TakeFocus((Widget) stw, event);
	StStartAction(stw, event);
	StAlterSelection(stw, StaticTextAdjust);
	StEndAction(stw);
}

static void
WriteToCB(StaticTextWidget stw)
{
        Atom	xa_primary = XA_PRIMARY;
	int	i;
	char	*buffer;
	char	*selstart, *selend;
	OlStrRep	rep = stw->primitive.text_format;
	
	StFetchSelection(stw, (unsigned char **)&selstart,
			 (unsigned char **)&selend);
	i = selend - selstart;
	if (stw->static_text.clip_contents != (char *)NULL)
		XtFree((char *)stw->static_text.clip_contents);

	stw->static_text.clip_contents = (OlStr)XtMalloc(i + wcsiz);
	buffer = (char *)stw->static_text.clip_contents;
	memmove((XtPointer)buffer, (XtPointer)selstart, i);
	if(rep != OL_WC_STR_REP)
		buffer[i] = '\0';
	else
		memmove((XtPointer)(buffer+i),&wcnull,wcsiz);

	if (!XtOwnSelection(	(Widget)stw,
				XA_CLIPBOARD(XtDisplay(stw)),
				stw->static_text.time,
				ConvertSelection,
				LoseClipboard,
				NULL))
		OlWarning(dgettext(OlMsgsDomain,
		(const char *)"stext:	We didn't get the selection!\n"));
}

/*
** This routine sets the start-of- and end-of-selection points, checks for
** multiclicks and increments the select mode appropriately,
*/

static void 
StStartSelection (StaticTextWidget stw, unsigned char *position, Boolean motion)
{
	static int last_time;

	if ( stw->static_text.time - last_time < 500) {
		stw->static_text.selection_mode =
				(stw->static_text.selection_mode + 1) % 
				NUM_SELECTION_TYPES;
	}

	else {
		stw->static_text.selection_mode = 0;
	}

	last_time = stw->static_text.time;

	stw->static_text.selection_start = position;
	stw->static_text.selection_end = position;
}

/*
** This routine just sets the selection end to the current position.
*/

static void 
StExtendSelection (StaticTextWidget stw, unsigned char *position)
{
	stw->static_text.selection_end = position;
}

/*
** This routine figures out what the widget's selection would be in
** the current mode {char, word, line, all}, and returns ptrs to the
** start and finish of it.
*/

static void
StFetchSelection (StaticTextWidget stw, unsigned char **start, unsigned char **finish)
{
	(*select_table[stw->static_text.selection_mode]) (
			stw->static_text.selection_start,
			stw->static_text.selection_end,
			start,
			finish,
			stw->static_text.output_string);
}

/*
**  The following family of functions implements different 
**  types of selection.
**  All are passed two unsigned char *'s indicating the two mouse positions
**  input by the user, two unsigned char **'s which will be set to
**  the real start and end of the selection in the current mode,
**  and a pointer to the start of the string.
**
**  The user-supplied start need not be less than the user-supplied end,
**  but the result start will be.
**
*/

/*
** Select char by char.
*/

static void
SelectChar(unsigned char *inputstart, unsigned char *inputend, unsigned char **start, unsigned char **end, unsigned char *stringstart)
{
	*start = _OlMin(inputstart, inputend);
	*end = _OlMax(inputstart, inputend);
}

/*
** Select word by word.
*/

static void
SelectWord(unsigned char *inputstart, unsigned char *inputend, unsigned char **start, unsigned char **end, unsigned char *stringstart)
{
	unsigned char 	*temp;
	wchar_t *wtemp;
	wchar_t *winputstart, *winputend;
	wchar_t *wstart, *wend, *wstringstart;
	int len;


switch(text_format) {
	case OL_SB_STR_REP:
	temp = _OlMin(inputstart, inputend) - 1;
	while (	temp >= stringstart && 
			*temp != ' ' && *temp != '\t' && *temp != '\n' ) {
		temp--;
	}

	*start = temp + 1;

	temp = _OlMax(inputstart, inputend);
	while (	*temp && *temp != ' ' && *temp != '\t' && 
		*temp != '\n') {
		temp++;
	}
	*end = temp;
	break;
	case OL_WC_STR_REP:
	wstringstart = (wchar_t *)stringstart;
	winputstart = (wchar_t *)inputstart;
	winputend = (wchar_t *)inputend;
	wtemp = _OlMin(winputstart, winputend) - 1;
	while (	wtemp >= wstringstart && 
			*wtemp != L' ' && *wtemp != L'\t' 
						&& *wtemp != L'\n' ) {
		wtemp--;
	}

	*start = (unsigned char *)(wtemp + 1);

	wtemp = _OlMax(winputstart, winputend);
	while (	*wtemp && *wtemp != L' ' && *wtemp != L'\t' && 
		*wtemp != L'\n') {
		wtemp++;
	}
	*end = (unsigned char *)wtemp;
	break;
	case OL_MB_STR_REP:
	temp = _OlMin(inputstart, inputend) - 1;
	while (	temp >= stringstart && 
			!(mblen((char *)temp,MB_CUR_MAX) == 1 && 
			(*temp == ' ' 
			|| *temp == '\t' 
			|| *temp == '\n') ) ) {
		temp--;
	}

	*start = temp + 1;

	temp = _OlMax(inputstart, inputend);
	while (	(len = mblen((char *)temp,MB_CUR_MAX)) > 1 ||
			(len == 1 
			&& *temp != ' ' 
			&& *temp != '\t' 
			&& *temp != '\n') ) {
		temp+=len;
	}
	*end = temp;
	break;
 } /* switch */

}

static void
SelectLine(unsigned char *inputstart, unsigned char *inputend, unsigned char **start, unsigned char **end, unsigned char *stringstart)
{
	unsigned char 	*temp;
	wchar_t *wtemp;
	wchar_t *winputstart, *winputend;
	wchar_t *wstringstart;
	int len;

switch(text_format) {
	case OL_SB_STR_REP:
	temp = _OlMin(inputstart, inputend) - 1;

	while (	(int)temp >= (int)stringstart && *temp != '\n') {
		temp--;
	}

	*start = temp + 1;

	temp = _OlMax(inputstart, inputend);
	while (	*temp && *temp != '\n') {
		temp++;
	}
	*end = temp;
	break;
	case OL_WC_STR_REP:
	winputstart = (wchar_t *)inputstart;
	winputend = (wchar_t *)inputend;
	wstringstart = (wchar_t *)stringstart;
	wtemp = _OlMin(winputstart, winputend) - 1;

	while (	wtemp >= wstringstart && *wtemp != L'\n') {
		wtemp--;
	}

	*start = (unsigned char *)(wtemp + 1);

	wtemp = _OlMax(winputstart, winputend);
	while (	*wtemp && *wtemp != L'\n') {
		wtemp++;
	}
	*end = (unsigned char *)wtemp;
	break;
	case OL_MB_STR_REP:
	temp = _OlMin(inputstart, inputend) - 1;

	while (	temp >= stringstart && 
		!(mblen((char *)temp,MB_CUR_MAX) == 1 
			&& *temp == '\n')) {
		temp--;
	}

	*start = temp + 1;

	temp = _OlMax(inputstart, inputend);
	while (	(len = mblen((char *)temp,MB_CUR_MAX)) > 1 ||
		(len ==1  && *temp != '\n') ) {
		temp+=len;
	}
	*end = temp;
	break;
  } /* switch */

}

static void
SelectAll(unsigned char *inputstart, unsigned char *inputend, unsigned char **start, unsigned char **end, unsigned char *stringstart)
{
wchar_t *winputstart, *winputend;
wchar_t *wend;
int len;

switch(text_format) {
	case OL_SB_STR_REP:
	*start = stringstart;
	*end = stringstart;
	
	while (	**end) {
		(*end)++;
	}
	break;
	case OL_WC_STR_REP:
		*start = stringstart;
		wend = (wchar_t *)stringstart;
		while(*wend) 
			wend++;

	        *end = (unsigned char *)wend;
	break;
	case OL_MB_STR_REP:
	*start = stringstart;
	*end = stringstart;
	
	while ((len = mblen((char *)*end,MB_CUR_MAX)) > 0) {
		(*end)+=len;
	}
	break;
}/* switch */
	
}

static void
StAlterSelection (StaticTextWidget stw, int mode)
                         
                 /* StaticTextStart, StaticTextAdjust, StaticTextEnd */
{
	unsigned char	*position;

	position = StPositionForXY (	stw, 
					(Position) stw->static_text.ev_x, 
					(Position) stw->static_text.ev_y,
				        (mode == StaticTextAdjust ?
						False: True));	

	if (position == 0)	/* not on any text */
		return;

	if (!XtOwnSelection(	(Widget)stw,
				XA_PRIMARY,
				stw->static_text.time,
				ConvertSelection,
				LosePrimary,
				NULL)) {

		printf("We didn't get the selection!\n");
	}

	switch (mode) {

	case StaticTextSelect: 
		StStartSelection (stw, (unsigned char *)position,True);
		StHighlightSelection(stw,False);
		break;

	case StaticTextAdjust: 
            	StExtendSelection (stw, (unsigned char *)position);
		StHighlightSelection(stw,False);
		break;
	}
}

/*
** This routine redisplays the minimum
** span of text necessary to indicate a new selection.
**
** Forceit is True if the current selection might be identical to the old
** one but we want to force full redisplay anyway (eg, called from Redisplay)
**
** If the new start and end are the same, there is no current selection.
*/

static void
StHighlightSelection(StaticTextWidget stw, Boolean forceit)
{
	unsigned char	*old_start = stw->static_text.oldsel_start;
	unsigned char	*old_end = stw->static_text.oldsel_end;
	unsigned char	*new_start, *new_end;
	Position	x,y;

	if(stw->static_text.selection_mode == NUM_SELECTION_TYPES + 1) {
		new_start = old_start;
		new_end = old_end;
		stw->static_text.selection_mode =
				stw->static_text.save_mode;
		stw->static_text.save_mode = 0;
	}
	else
		 StFetchSelection(stw,&new_start, &new_end);

	if ((forceit != True) && (old_start == new_start && old_end == new_end))
		return;			/* new selection identical to old */

	if (!new_start || !new_end)	/* no selection set */
		return;
/*
** First selection ever?
*/
	if (!old_start && !old_end) {
		StDisplaySubstring(	stw,
					_OlMin(new_start,new_end),
					_OlMax(new_start,new_end),
					stw->static_text.hilite_GC,
					True);
	} else if (new_start == new_end) {
		StDisplaySubstring(	stw,
					old_start,
					old_end,
					stw->static_text.normal_GC,
					True);
	}
	else {
		StDisplaySubstring(	stw,
					old_start,
					old_end,
					stw->static_text.normal_GC,
					True);
		StDisplaySubstring(	stw,
					new_start,
					new_end,
					stw->static_text.hilite_GC,
					True);
	}

	stw->static_text.oldsel_start = new_start;
	stw->static_text.oldsel_end = new_end;
}

/*
** Given a static text widget and a char * within it, return the 
** (x,y) position of the lower left (left to right) or 
** the lower right hand corner (right to left ) 
** corner of the specified character.
*/

static void
StXYForPosition(StaticTextWidget stw, unsigned char *position, Position *px, Position *py)
{
	XFontStruct	*font;
	XFontSet	fontset;
	XRectangle	*cliprect;
	Dimension	delta;
	int	line,esc;
	StaticTextPart	*stp = &(stw->static_text);
	XRectangle ink_ret;
	XRectangle logical_ret;
	XFontSetExtents *fset_extents;

	if(stw->primitive.text_format == OL_SB_STR_REP)
		font =  (XFontStruct *)(stw->primitive.font);
	else {
		fontset =  (XFontSet)(stw->primitive.font);
		fset_extents = XExtentsOfFontSet(fontset);
	}
	cliprect = &(stp->TextRect);
/*
** Figure out which line the char belongs to, and set the Y value.
*/
	line = 0; 
	while (	stp->line_table[line] &&
			position >= stp->line_table[line]) {
		line++;
	}

	line--;

	if(stw->primitive.text_format == OL_SB_STR_REP){
		delta = ((stp->line_space / 100.0) + 1) * 
					(font->descent + font->ascent);
		*py = cliprect->y + 
			(Position) ((line * delta) + font->ascent);
	}
	else if(stw->primitive.text_format == OL_WC_STR_REP){
		XwcTextExtents(fontset,(wchar_t *)position,1,
                           		&ink_ret, &logical_ret);
		delta = ((stp->line_space / 100.0) + 1) * 
				fset_extents->max_logical_extent.height;
		*py = cliprect->y + (Position) ((line * delta) + 
						(-logical_ret.y));
	}
	else{
		XmbTextExtents(fontset,(char *)position,
				(int)mblen((char *)position,MB_CUR_MAX),
                           			&ink_ret, &logical_ret);
		delta = ((stp->line_space / 100.0) + 1) * 
				fset_extents->max_logical_extent.height;
		*py = cliprect->y + (Position) ((line * delta) + 
						(-logical_ret.y));
	}


/*
** And what is the X-coordinate for this specific char?
*/

	if(stw->primitive.text_format == OL_SB_STR_REP){
		*px =(Position) StGetOffset(stw, line)
			+ XTextWidth(font, (const char *)stp->line_table[line],
				     position - stp->line_table[line]);
	}
	else if(stw->primitive.text_format == OL_WC_STR_REP){
		esc = XwcTextExtents(fontset, (wchar_t *)stp->line_table[line],
                         (int)(position - stp->line_table[line])/
				wcsiz,&ink_ret, &logical_ret);
				
		if(logical_ret.x >= 0)
		*px =(Position) StGetOffset(stw, line) + esc;			
		else
		*px =(Position) StGetOffset(stw, line) - esc;			
	}
	else{
		esc = XmbTextExtents(fontset, (const char *)
				     stp->line_table[line],
                         (position - stp->line_table[line]),
					&ink_ret, &logical_ret);
		if(logical_ret.x >= 0)
		*px =(Position) StGetOffset(stw, line) + esc;			
		else
		*px =(Position) StGetOffset(stw, line)  - esc;			
	}
}

/*
** Figure the X offset for the given line with the widget's alignment.
*/

static Position 
StGetOffset(StaticTextWidget stw, int line)
{
	Dimension xoff;
	int len;
	StaticTextPart	*stp = &(stw->static_text);
	XRectangle	*cliprect = &(stp->TextRect);
	Dimension	maxwin = stw->core.width - (2 * stp->internal_width)
				- (2 * stw->static_text.highlight_thickness);
	unsigned char	*str1;
	wchar_t         *wstr1;
	XFontStruct	*font;
	XFontSet	fontset;
	XRectangle      ink_ret;
	XRectangle	logical_ret;
	int		esc;
	unsigned char *pline = stp->line_table[line];
	int i =	stp->line_lens[line];

switch(stw->primitive.text_format) {
   case OL_SB_STR_REP:
	font = (XFontStruct *)stw->primitive.font;
	switch (stp->alignment) {
	case OL_LEFT:
		xoff = 0;
		break;

	case OL_CENTER:
		esc = XTextWidth(font, (const char *)pline, i);
		if(esc >= 0)
		xoff = (Position) ((int)(cliprect->width -esc)/ 2 );
		else
		xoff = (Position) ((int)(cliprect->width -(-esc))/ 2 +
						(-esc));
		break;

	case OL_RIGHT:
		esc = XTextWidth(font, (const char *)pline, i);
		if(esc >= 0)
		xoff = (Position) ((int)(cliprect->width -esc));
		else
		xoff = (Position) (int)(cliprect->width);
		break;
	default:  
		OlWarning(dgettext(OlMsgsDomain,
		"StaticTextWidget: Ilegal alignment specified\n"));
		xoff = 0;
		break;
	}
	break;
   case OL_WC_STR_REP:
	fontset = (XFontSet)stw->primitive.font;
	switch (stp->alignment) {
	case OL_LEFT:
		xoff = 0;
		break;

	case OL_CENTER:
		wstr1 = (wchar_t *)pline;
		esc = XwcTextExtents(fontset,
				wstr1,i,&ink_ret,&logical_ret);
		xoff = (Position)((int)(cliprect->width - esc)/2 -
					logical_ret.x);
					 
		break;

	case OL_RIGHT:
		wstr1 = (wchar_t *)pline;
		esc = XwcTextExtents(fontset,
				wstr1,i,&ink_ret,&logical_ret);
		xoff = (Position)(cliprect->width - esc - logical_ret.x);
		break;
	default:  
		OlWarning(dgettext(OlMsgsDomain,
		"StaticTextWidget: Ilegal alignment specified\n"));
		xoff = 0;
		break;
	}
	break;
   case OL_MB_STR_REP:
	fontset = (XFontSet)stw->primitive.font;
	switch (stp->alignment) {
	case OL_LEFT:
		xoff = 0;
		break;

	case OL_CENTER:
		esc = XmbTextExtents(fontset, (const char *)pline,
				     i,&ink_ret,&logical_ret);
		xoff = (Position)((int)(cliprect->width - esc)/2 -
					logical_ret.x);
		break;

	case OL_RIGHT:
		esc = XmbTextExtents(fontset, (const char *)pline,
				     i,&ink_ret,&logical_ret);
		xoff = (Position)(cliprect->width - esc - logical_ret.x);
		break;
	default:  
		OlWarning(dgettext(OlMsgsDomain,
		"StaticTextWidget: Ilegal alignment specified\n"));
		xoff = 0;
		break;
	}
	break;
  }

	if (cliprect->width > maxwin)  {
		if (stp->alignment == OL_CENTER)
			xoff -= (int) (cliprect->width - maxwin)/2;
		else if (stp->alignment == OL_RIGHT)
			xoff -= cliprect->width - maxwin;
	}

	xoff += (Position) (cliprect->x);
	return (xoff);
}

/* 
 * This routine maps an x and y position in a window that is displaying text
 * into the corresponding char * into the source.
 */

/*--------------------------------------------------------------------------+*/
static unsigned char *
StPositionForXY (StaticTextWidget stw, Position x, Position y, Boolean at_start)
/*--------------------------------------------------------------------------+*/
                     
             
                 
{
	int	i, line,len;
	Dimension	y_delta; 
	int	templine;
	unsigned char	*tempstring;
	wchar_t 	*wtemps;
	unsigned char	*result;
	StaticTextPart	*stp;
        XFontSetExtents *fset_extents;
	XFontStruct	*font;
	XFontSet	fontset;

	stp = &(stw->static_text);

	if(stw->primitive.text_format == OL_SB_STR_REP) 
		font = (XFontStruct *)(stw->primitive.font);
	else
		fontset = (XFontSet)(stw->primitive.font);
/*
** Ensure the event is not outside the widget (This can happen with drags)
*/

	if (x < 0 || x > (Position) stw->core.width) {
		return (0);
	}
	
	if (y < 0 || y > (Position) stw->core.height) {
		return (0);
	}

/*
** Now figure out which line we're on...
*/

	if(stw->primitive.text_format == OL_SB_STR_REP){
		y_delta = (Dimension) (((stp->line_space / 100.0) + 1) * 
			(font->descent + font->ascent));
	}
	else if(stw->primitive.text_format == OL_WC_STR_REP){
		fset_extents = XExtentsOfFontSet(fontset);
		y_delta = (Dimension) (((stp->line_space / 100.0) + 1) * 
				fset_extents->max_logical_extent.height);
	}
	else{
		fset_extents = XExtentsOfFontSet(fontset);
		y_delta = (Dimension) (((stp->line_space / 100.0) + 1) * 
				fset_extents->max_logical_extent.height);
	}


	if (y_delta != 0) {
		for (	line = 0; 
			(int) ((line+1) * y_delta) < (int) (y - stp->TextRect.y);
			(line)++)
			;
	}
	else {
		line = 0;
	}

	/*
	** If line is 'beyond' the intialized string table, the user
	** clicked in "no-mans" land!
	*/
	if (line >= stp->line_count)
		return(0);

	tempstring = stp->line_table[line];

	if (!tempstring)
		return (0);

/*
** now hunt for the right character within the line...
*/
	x -= (Position) StGetOffset(stw,line);

/*************************** encoding specific part !! ***********************/

	switch(stw->primitive.text_format) {
	case OL_SB_STR_REP:
		for (i = 0; tempstring[i] && 
			XTextWidth(font,(const char *)tempstring,i+1) < x; i++)
				if (tempstring[i] == '\n') 
					break;
		if(!at_start) i++;
		result = tempstring + i;
		break;
	case OL_WC_STR_REP:
		wtemps = (wchar_t *)tempstring;
		for (i = 0; wtemps[i] && 
			XwcTextEscapement(fontset,wtemps,i+1) < x; i++)
				if (wtemps[i] == L'\n')
					break;
		if(!at_start) i++;
		result = (unsigned char *)&wtemps[i];
		break;
 	case OL_MB_STR_REP:	
		for (i = 0;(len = mblen((char *)&tempstring[i],
				MB_CUR_MAX)) >0 &&  
			XmbTextEscapement(fontset,(const char *)tempstring,
					  i+len) < x; i+=len)
				if (len == 1 && tempstring[i] == '\n') 
					break;
		if(!at_start) i+=len;
		result = tempstring + i;
		break;
	}

return(result);
}

static void
StStartAction(StaticTextWidget stw, XEvent *event)
{
	if (event) {
		stw->static_text.time = event->xbutton.time;
		stw->static_text.ev_x = event->xbutton.x;
		stw->static_text.ev_y = event->xbutton.y;
	}
}

static void
StEndAction(StaticTextWidget stw)
{
}

/*************************************<->*************************************
 *
 *	static void StDisplaySubstring(stw,start,finish,gc)
 *		StaticTextWidget	stw;
 *		char	*start, *finish;
 *		GC	gc;
 *
 *	Description:
 *		Display the string from "start" to "finish" with the
 *		supplied GC.  Include "start" but not "finish".
 *
 *************************************<->***********************************/
static void 
StDisplaySubstring(StaticTextWidget stw, unsigned char *start, unsigned char *finish, GC gc, Boolean isSensitive)
{
	int	i;
	Position	cur_x, cur_y;	/* Current start of baseline */
	Dimension	y_delta;	/* Absolute space between baselines */
	Dimension	x_delta;	/* Left bearing of first char in line */
	unsigned char	*str1, *str2, *str3;
	wchar_t		*wstr1, *wstr2, *wstr3;
	int		len;
	XFontStruct	*font;
	XFontSet	fontset;
	XCharStruct	overall;
	StaticTextPart	*stp;
	int		(*drawFunc)(Display *display, Drawable d, GC gc,
				    int x, int y, const char *string,
				    int length);
	void		(*wcDrawFunc)(Display *display, Drawable d,
				      XFontSet font_set, GC gc,
				      int x, int y, wchar_t *text,
				      int num_wchars);
	void		(*mbDrawFunc)(Display *display, Drawable d,
				      XFontSet font_set, GC gc,
				      int x, int y, const char *text,
				      int bytes_text);

	stp = &(stw->static_text);

/*
 * Watch out for null strings.
 */
	/* XDrawImageString enforces fillstyle = FillSolid (ref man).
	 * Hence we cannot use this for drawing insensitive text.
	 * However, its quite handy for drawing selected/highlighted
	 * stuff. So we switch between these two funcs as reqd .. -JMK
	 */

switch(stw->primitive.text_format) {
case OL_SB_STR_REP:
	font = (XFontStruct *)stw->primitive.font;
	str1 = str2 = start;
	if (!start || !*start)
		return;
	drawFunc = (isSensitive ? XDrawImageString : XDrawString);

	while(*str1 && (str1 < finish)) {

		StXYForPosition(stw, str1, &cur_x, &cur_y);
		str2 = str1;
		for(	i = 0; 
			(*str1 && (*str1 != '\n') && (str1 < finish)); 
			i++, str1++)
			;
		if (i) 
		(*drawFunc)(XtDisplay(stw), XtWindow(stw),
			    gc, cur_x, cur_y, (const char *)str2,i);
		if (*str1)
			str1++; /* Step past the \n */
	}
	break;
case OL_WC_STR_REP:
	fontset = (XFontSet)stw->primitive.font;
	wstr1 = wstr2 = (wchar_t *)start;
	if (!wstr1 || !*wstr1)
		return;
	wcDrawFunc = (isSensitive ? XwcDrawImageString : XwcDrawString);

	while(*wstr1 && (wstr1 < (wchar_t *)(finish))) {
		StXYForPosition(stw, (unsigned char *)wstr1, 
						&cur_x, &cur_y);
		wstr2 = wstr1;
		for(i = 0; 
			(*wstr1 && (*wstr1 != L'\n') && 
					(wstr1 < (wchar_t *)finish)); 
			i++, wstr1++)
			;
		if (i) 
		(*wcDrawFunc)(XtDisplay(stw), XtWindow(stw),
				fontset, gc,cur_x, cur_y, wstr2,i);
		if (*wstr1)
			wstr1++; /* Step past the \n */
	}
	break;
case OL_MB_STR_REP:
	fontset = (XFontSet)stw->primitive.font;
	str1 = str2 = start;
	if (!start || !*start)
		return;
	mbDrawFunc = (isSensitive ? XmbDrawImageString : XmbDrawString);

	while(mblen((char *)str1, MB_CUR_MAX) > 0
				 && (str1 < finish)) {
		StXYForPosition(stw, str1, &cur_x, &cur_y);
		str2 = str1;
		for(i = 0;(((len = mblen((char *)str1,MB_CUR_MAX))
				> 1 || (len ==1 && 
				*str1 != '\n')) && (str1 < finish)); 
			i+=len, str1+=len)
			;
		if (i) 
		(*mbDrawFunc)(XtDisplay(stw), XtWindow(stw),
			      fontset,gc, cur_x, cur_y, (const char *)str2,i);
		if (len ==1 && *str1 == '\n')
			str1++; /* Step past the \n */
	}
	break;
  } /* switch */
}

static void
StBuildLineTable(StaticTextWidget stw)
{
	unsigned char	*str1;
	wchar_t *wstr1;
	int	cur_line = 0;
	int	i,len;
	char	c;
	StaticTextPart	*stp = &(stw->static_text);

switch(stw->primitive.text_format) {
   case OL_SB_STR_REP:
	str1 = (unsigned char *)(stw->static_text.output_string);
	while (*str1) {
		stp->line_table[cur_line] = str1;
		for( i = 0; ( *str1 && (*str1 != '\n')); i++, str1++)
			;
		stp->line_lens[cur_line++] = i;
		if(*str1 == '\n')
			str1++;
		if (cur_line >= stp->line_count) {
			stp -> line_count += stp->line_count;
			stp->line_table =  (unsigned char **)
				XtRealloc((char *) (stp->line_table), 
					   stp->line_count * sizeof(char *));
			stp->line_lens =  (int*) XtRealloc 
			    ((char *)stp->line_lens, 
			     stp->line_count*sizeof(int));
		}
	}
	for (i = cur_line; i < stp->line_count; i++) {
		stp->line_lens[i] = 0;
		stp->line_table[i] = NULL;
	}
	break;
   case OL_WC_STR_REP:
	wstr1 = (wchar_t *)stw->static_text.output_string;
	while (*wstr1) {
		stp->line_table[cur_line] = (unsigned char *)wstr1;
		for( i = 0; ( *wstr1 && (*wstr1 != L'\n')); i++, wstr1++)
			;
		stp->line_lens[cur_line++] = i;
		if(*wstr1 == L'\n')
			wstr1++;
		if (cur_line >= stp->line_count) {
			stp -> line_count += stp->line_count;
			stp->line_table =  (unsigned char **)
				XtRealloc ((char *) (stp->line_table), 
					stp->line_count * sizeof(char *));
			stp->line_lens =  (int*) XtRealloc 
			    ((char *)stp->line_lens, 
			     stp->line_count*sizeof(int));
		}
	}
	for (i = cur_line; i < stp->line_count; i++) {
		stp->line_lens[i] = 0;
		stp->line_table[i] = NULL;
	}
	break;
   case OL_MB_STR_REP:
	str1 = (unsigned char *)stw->static_text.output_string;
	while (mblen((char *)str1,MB_CUR_MAX) > 0) {
		stp->line_table[cur_line] = str1;
		for( i = 0; ((len = mblen((char *)str1, MB_CUR_MAX))
				> 1 || (len ==1 && *str1 != '\n')); 
						i+=len, str1+=len)
			;
		stp->line_lens[cur_line++] = i;
		if(*str1 == '\n')
			str1++;
		if (cur_line >= stp->line_count) {
			stp -> line_count += stp->line_count;
			stp->line_table =  (unsigned char **)
				XtRealloc ((char *)(stp->line_table), 
					   stp->line_count * sizeof(char *));
			stp->line_lens =  (int*) XtRealloc 
			    ((char *)stp->line_lens, 
			     stp->line_count*sizeof(int));
		}
	}
	for (i = cur_line; i < stp->line_count; i++) {
		stp->line_lens[i] = 0;
		stp->line_table[i] = NULL;
	}
	break;
  } /* switch */
}

/*
 *  ActivateWidget - this routine is used to activate the text related
 *			operation.
 *
 *		     currently, it only handles OL_COPY.
 */

static Boolean
ActivateWidget (Widget w, OlVirtualName type, XtPointer call_data)
{
	Boolean consumed = False;

	switch (type)
	{
		case OL_COPY:
			consumed = True;
			WriteToCB((StaticTextWidget)w);
			break;
		default:
			break;
	}
	return (consumed);
} /* end of ActivateWidget */

/*
 *  HandleButton: this routine handles the ButtonPress and ButtonRelease
 *			events.
 *
 *	note: now this widget is only interested in a perfect match
 *		of a coming event. e.g., ctrl<selectBtn> won't come
 *		in as OL_SELECT, this is the only difference between
 *		the old way and this one.
 */
static void
HandleButton (Widget w, OlVirtualEvent ve)
{
	StaticTextPart *stp = &(((StaticTextWidget)w)->static_text);

	switch (ve->virtual_name)
	{
		case OL_SELECT:
			ve->consumed = True;
			if (stp->selectable && ve->xevent->type == ButtonPress)
				SelectStart ((StaticTextWidget)w, ve->xevent);
			break;
		case OL_ADJUST:
			if (stp->selectable && ve->xevent->type == ButtonRelease)
			{
				ve->consumed = True;
				SelectAdjust((StaticTextWidget)w, ve->xevent);
			}
			break;
		default:
			break;
	}
} /* end of HandleButton */

/*
 *  HandleMotion: this routine handles the Montion events
 */
static void
HandleMotion(Widget w, OlVirtualEvent ve)
{
	StaticTextPart *stp = &(((StaticTextWidget)w)->static_text);

	switch (ve->virtual_name)
	{
		case OL_SELECT:
		case OL_ADJUST:
			if (stp->selectable) {
				ve->consumed = True;
				SelectAdjust((StaticTextWidget)w, ve->xevent);
			}
			break;
		default:
			break;
	}
} /* end of HandleMotion */

static void
TakeFocus(Widget w, XEvent *event)
{
#define HAS_FOCUS(w)	(((StaticTextWidget)(w))->primitive.has_focus == TRUE)

	if (!HAS_FOCUS(w) && _OlMouseless(w))
	{
#if 1 /* take it out after removing pointer warping for the dft highlighting */
		Window		junk_win;
		int		junk_xy, x, y;
		unsigned int	junk_mask;

		XQueryPointer(XtDisplay(w), XtWindow(w), &junk_win,
				&junk_win, &junk_xy, &junk_xy,
				&x, &y, &junk_mask);
		(void) OlCallAcceptFocus((Widget)w, event->xbutton.time);
		XWarpPointer(XtDisplay(w),None,XtWindow(w),0,0,0,0,x,y);
#else
		(void) OlCallAcceptFocus(w, event->xbutton.time);
#endif
	}
#undef HAS_FOCUS
} /* end of TakeFocus */

/* Routine to strip leading/trailing spaces from a buffer, which can
 * contain a numer of lines -JMK 
*/
static void
StripSpaces(StaticTextWidget stw)
{
        StaticTextPart *stp = &(stw->static_text);
        int alignment = stw->static_text.alignment;
        unsigned char *p1,*p2;
	wchar_t *wp1, *wp2;
	int i =0, len;
	int nspaces = 0;
	

switch(stw->primitive.text_format) {
	case OL_SB_STR_REP:
        p1 = p2 = (unsigned char *)stp->output_string;

        while (*p1) {
                if ((alignment == OL_LEFT) || (alignment == OL_CENTER))
                        while (*p1 == ' ') p1++;

		i = 0;
                while ((*p1 != '\0') && (*p1 != '\n'))
                        i++, *p2++ = *p1++;

		if(i) {
                	while (*--p2 == ' ');
                	p2++;
		}

		if(*p1 == '\n' && *(p1+1) == '\0') 
			*p2++ = *p1++;

		*p2 = *p1;

                if (*p1 != '\0') 
                	p1++ , p2++;
        }

	break;
	case OL_WC_STR_REP:
        wp1 = wp2 = (wchar_t *)stp->output_string;

        while (*wp1) {
                if ((alignment == OL_LEFT) || (alignment == OL_CENTER))
                        while (*wp1 == L' ') wp1++;

		i = 0;
                while ((*wp1 != wcnull) && (*wp1 != L'\n'))
                        i++, *wp2++ = *wp1++;

		if(i) {
                	while (*--wp2 == L' ');
                	wp2++;
		}

		if(*wp1 == L'\n' && *(wp1+1) == wcnull) 
			*wp2++ = *wp1++;

		*wp2 = *wp1;

                if (*wp1 != wcnull) 
                	wp1++ , wp2++;
        }

	break;
	case OL_MB_STR_REP:
        p1 = p2 = (unsigned char *)stp->output_string;

        while (mblen((char *)p1,MB_CUR_MAX) > 0) {
                if ((alignment == OL_LEFT) || (alignment == OL_CENTER))
                        while (mblen((char *)p1,MB_CUR_MAX) == 1 &&
							*p1 == ' ') 
				p1++;

		i = 0;
                while ((len = mblen((char *)p1, MB_CUR_MAX)) > 1 ||
			(len ==1 && (*p1 != '\n'))) {
			if(len == 1 && *p1 == ' ')
				nspaces++;
			else
				nspaces = 0;
                        i+=len;
			memmove((XtPointer)p2,(XtPointer)p1,len);
			p2 += len;
			p1 += len;
		}

		if(i)
			p2 -= nspaces;

		/* len must be 1 in case of '\n' */
		if(len == 1 && *p1 == '\n' && 
			mblen((char *)(p1+1),MB_CUR_MAX) == 0) {
			*p2++ = *p1++;
			len = 0;
		}

		*p2 = *p1;

                if (len == 1) 
                	p1++ , p2++;
        }

	break;
  }

}
