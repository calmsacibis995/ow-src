#pragma ident	"@(#)TextEPos.c	302.19	97/03/26 lib/libXol SMI"	/* textedit:TextEPos.c 1.12	*/

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


#include <libintl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include <Xol/Dynamic.h>
#include <Xol/Oltextbuff.h>
#include <Xol/OlI18nP.h>
#include <Xol/OlStrMthdsI.h>
#include <Xol/OpenLookP.h>
#include <Xol/RootShell.h>
#include <Xol/Scrollbar.h>
#include <Xol/TextDisp.h>
#include <Xol/TextEPos.h>
#include <Xol/TextEdit.h>
#include <Xol/TextEditP.h>
#include <Xol/TextUtil.h>
#include <Xol/TextWrap.h>
#include <Xol/Util.h>
#include <Xol/buffutil.h>
#include <Xol/memutil.h>
#include <Xol/textbuff.h>


static Boolean          ConvertPrimary(Widget w, Atom *selection, Atom *target, Atom *type_return, XtPointer *value_return, long unsigned int *length_return, int *format_return);
static void             LosePrimary(Widget w, Atom *atom);

static void    _MoveCursorPositionSB(
		      TextEditWidget ctx,
		      XEvent *       event,
		      OlInputEvent   direction_amount,
		      TextPosition   newpos);
static void    _MoveCursorPositionMBWC(
		      TextEditWidget ctx,
		      XEvent *       event,
		      OlInputEvent   direction_amount,
		      TextPosition   newpos);
static void	_ExtendSelectionSB(
		      TextEditWidget ctx,
		      OlInputEvent   ol_event);
static void	_ExtendSelectionMBWC(
		      TextEditWidget ctx,
		      OlInputEvent   ol_event);
static Boolean	_MoveSelectionSB(
		      TextEditWidget ctx,
		      TextPosition   position,
		      TextPosition   selectStart,
		      TextPosition   selectEnd,
		      int            selectMode);
static Boolean	_MoveSelectionMBWC(
		      TextEditWidget ctx,
		      TextPosition   position,
		      TextPosition   selectStart,
		      TextPosition   selectEnd,
		      int            selectMode);


#define HAS_FOCUS(w)	(((TextEditWidget)(w))->primitive.has_focus)
#define ACTIVE_CARET(w)	(HAS_FOCUS(w) && ((((TextEditWidget)(w))->textedit.editMode == OL_TEXT_EDIT)))


/*
 * ConvertPrimary:
 *	
 *	XA_STRING is always provided as a multibyte string. We now support
 *  COMPOUND_TEXT too ....
 *
 */
static Boolean
ConvertPrimary(Widget  	       w,
	       Atom *          selection,
	       Atom *          target,
	       Atom *          type_return,
	       XtPointer *     value_return,
	       unsigned long * length_return,
	       int *           format_return)
{
    TextEditWidget ctx = (TextEditWidget)w;
    TextEditPart	*text  = &ctx-> textedit;
    Boolean		retval = False;
    Atom		*atoms = NULL;
    OlStr		ptr;
    Atom		atom;
    Display		*dpy = XtDisplay(ctx);

    if (*selection == XA_PRIMARY) {
	if (*target == OlInternAtom(dpy, TARGETS_NAME)) {
	    *format_return = (int)(8*sizeof(Atom));
	    *length_return = (unsigned long)6;
	    atoms          = (Atom *)MALLOC((unsigned)((*length_return)*(sizeof(Atom))));
	    atoms[0]       = OlInternAtom(dpy, TARGETS_NAME);
	    atoms[1]       = OlInternAtom(dpy, _OL_COPY_NAME);
	    atoms[2]       = OlInternAtom(dpy, _OL_CUT_NAME);
	    atoms[3]       = XA_STRING;
	    atoms[4]       = OlInternAtom(dpy, DELETE_NAME);
	    atoms[5]       = OlInternAtom(dpy, COMPOUND_TEXT_NAME);
	    *value_return  = (XtPointer)atoms;
	    *type_return   = XA_ATOM;
	    
	    retval         = True;
	} else if (*target == (atom = OlInternAtom(dpy, _OL_COPY_NAME))) {
	    (void) OlTextEditCopySelection(ctx, False);
	    *format_return = NULL;
	    *length_return = NULL;
	    *value_return = NULL;
	    *type_return = atom;
	} else if (*target == (atom = OlInternAtom(dpy, _OL_CUT_NAME))) {
	    (void) OlTextEditCopySelection(ctx, True);
	    *format_return = NULL;
	    *length_return = NULL;
	    *value_return = NULL;
	    *type_return = atom;
	} else if (*target == XA_STRING) {
	    OlStrRep	rep = text->text_format;
		OlStr   	cvt_str;

		if (text-> selectEnd != text-> selectStart)	{ /* Selection exists */
			OlTextEditReadSubString(ctx, (char **)&ptr,
					text->selectStart, text->selectEnd - 1);
			if (rep == OL_WC_STR_REP) { /* WC - convert to MB */			
				size_t size = sizeof(char) * MB_CUR_MAX *
							(wslen((wchar_t *)ptr)+1);
				cvt_str = (char *)XtMalloc(size);
				(void)wcstombs(cvt_str, (wchar_t *)ptr, size);
				XtFree(ptr);
			} else
				cvt_str = ptr;
			
			*format_return = 8;
			*length_return = strlen((const char *)cvt_str) +1;
			*value_return = (XtPointer)cvt_str;
			*type_return = XA_STRING;
			retval = True;
		} else
			retval = False;

	} else if (*target == OlInternAtom(dpy, COMPOUND_TEXT_NAME)) {
	    int	        cvt_len;
	    OlStr	cvt_str;
	    OlStrRep	rep = text->text_format;
      
		if (text-> selectEnd != text-> selectStart)	{ /* Selection exists */
			OlTextEditReadSubString(ctx, (char **)&ptr,
					text->selectStart, text->selectEnd - 1);

	    	cvt_str = str_methods[rep].StrToCT(XtDisplay(w), ptr, &cvt_len);
	    	*format_return = 8;
	    	*value_return = (XtPointer)cvt_str;
	    	*length_return = cvt_len;
	    	*type_return = (Atom)*target;
			retval = True;
		} else
			retval = False;
	    
	} else if (*target == OlInternAtom(dpy, DELETE_NAME)) {
	    *format_return = NULL;
	    *length_return = NULL;
	    *value_return = NULL;
	    *type_return = OlInternAtom(dpy, DELETE_NAME);
	} else if (*target == OlInternAtom(dpy, _SUN_SELN_YIELD_NAME)) {
	    LosePrimary((Widget)ctx, selection);
	    *format_return = NULL;
	    *length_return = NULL;
	    *value_return = NULL;
	    *type_return = NULL;
	    *target = NULL;
	    retval = False;
	} else 
	    OlWarning(dgettext(OlMsgsDomain,
			       "TextEdit: Can't convert PRIMARY"));
    }
    return (retval);

} /* end of ConvertPrimary */

/*
 * LosePrimary
 *
 */

static void
LosePrimary(Widget w, Atom *atom)
{
	TextEditWidget  ctx = (TextEditWidget)w;

_MoveSelection(ctx, ctx-> textedit.cursorPosition, 0, 0, 0);

} /* end of LosePrimary */
/*
 * _SetDisplayLocation
 *
 * The \fI_SetDisplayLocation\fR procedure is used to set the
 * displayLocation of the TextEdit widget \fIctx\fR to a \fInew\fR
 * wrap location.  This location is \fIdiff\fR wrap lines from the
 * current display wrap location.
 *
 * See also:
 *
 * _SetTextXOffset(3)
 *
 * Synopsis:
 *
 *#include <TextPos.h>
 * ...
 */

extern void
_SetDisplayLocation(TextEditWidget ctx, TextEditPart *text, int diff, int setSB, TextLocation new)
{
    OlStrRep	    rep        = text->text_format;
    TextBuffer *    textBuffer = text->textBuffer;
    WrapTable *     wrapTable = text->wrapTable;

    text-> displayLocation = _LocationOfWrapLocation(text-> wrapTable, new);
    text->displayPosition  = (text->text_format == OL_SB_STR_REP)?
	PositionOfLocation(text-> textBuffer, text->displayLocation):
	OlPositionOfLocation((OlTextBufferPtr)text-> textBuffer, &(text->displayLocation));
    if (diff != 0) {
	if (setSB && text-> vsb != NULL) {
	    Arg			arg[1];
	    int			sliderValue;
	    TextLocation	loc;

	    loc = _IncrementWrapLocation(wrapTable, new, diff,
	       (diff > 0)? _LastWrapLine(wrapTable):_FirstWrapLine(wrapTable),
	       NULL);			 
	    sliderValue = _PositionOfWrapLocation(textBuffer, wrapTable, loc);
	    XtSetArg(arg[0], XtNsliderValue, sliderValue);
				   
	    XtSetValues(text-> vsb, arg, 1);
	}
    }
} /* end of _SetDisplayLocation */

/*
 * _SetTextXOffset
 *
 * The \fI_SetTextXOffset\fR procedure is used to increment the xOffset
 * of the TextEdit widget \fIctx\fR by \fIdiff\fR pixels.
 *
 * See also:
 *
 * _SetDisplayLocation(3)
 *
 * Synopsis:
 *
 *#include <TextPos.h>
 * ...
 *
 */

extern void
_SetTextXOffset(TextEditWidget ctx, TextEditPart *text, int diff, int setSB)
{

if (diff != 0)
   {
   text-> xOffset -= diff;
   if (setSB && text-> hsb != NULL)
      {
      Arg arg[2];
      int sliderValue;

      XtSetArg(arg[0], XtNsliderValue, &sliderValue);
      XtGetValues(text-> hsb, arg, 1);

      sliderValue += diff;

      XtSetArg(arg[0], XtNsliderValue, sliderValue);
      XtSetArg(arg[1], XtNsliderMax,   text-> maxX);
      XtSetValues(text-> hsb, arg, 2);
      }
   }

} /* end of _SetTextXOffset */

/*
 * _CalculateCursorRowAnXOffset
 *
 */
extern void
_CalculateCursorRowAndXOffset(TextEditWidget ctx, int *row, int *xoffset, TextLocation currentDP, TextLocation currentIP)
{
    TextEditPart * text          = &(ctx-> textedit);
    WrapTable *    wrapTable     = text-> wrapTable;
    int            x;
    OlStr          p;
    int            diff;
    OlStrRep	   str_rep = text->text_format;
    int		   lmargin;
    UnitPosition   curs_upos;

    (void) _MoveToWrapLocation(wrapTable, currentDP, currentIP, row);

    if (str_rep == OL_SB_STR_REP) {
	p = (OlStr)GetTextBufferLocation(text-> textBuffer, currentIP.line, NULL);
	curs_upos = text->cursorLocation.offset - 1;
    } else {
	TextLocation loc = text->cursorLocation;
	
	p = OlGetTextBufferStringAtLine((OlTextBufferPtr)text->textBuffer, currentIP.line, NULL);
	if (loc.offset > 0) {
	    loc.offset--;
	    curs_upos = OlUnitOffsetOfLocation((OlTextBufferPtr)text->textBuffer, &loc);
	} else
	    curs_upos = -1;
    }

    x = _StringWidth(0, p, _WrapLineUnitOffset(text->textBuffer, wrapTable, currentIP),
		     curs_upos, ctx->primitive.font, str_rep, text->tabs)
	+ PAGE_L_MARGIN(ctx) + text->xOffset;
    lmargin = (int)PAGE_L_MARGIN(ctx);
    if (x < lmargin)
	diff = MAX(x - lmargin - HORIZONTAL_SHIFT(ctx), text-> xOffset);
    else
	if (x <= PAGE_R_MARGIN(ctx))
	    diff = 0;
	else {
	    int  pwid = PAGEWID(ctx);
	    
	    diff = MIN(text-> maxX + text-> xOffset - pwid, 
		       x + HORIZONTAL_SHIFT(ctx) - pwid);
	}
    *xoffset = diff;

} /* end of _CalculateCursorRowAndXOffset */

/*
 * _MoveDisplayPosition
 *
 */
extern int
_MoveDisplayPosition(TextEditWidget ctx, XEvent *event, OlInputEvent direction_amount, TextPosition newpos)
{
    TextEditPart * text       = &ctx-> textedit;
    WrapTable *    wrapTable  = text-> wrapTable;
    int            page       = LINES_VISIBLE(ctx);
    int            diff       = 0;
    TextLocation   current;
    TextLocation   minpos;
    TextLocation   maxpos;
    TextLocation   new;
    TextPosition   delta;

    current = _WrapLocationOfLocation(wrapTable, text-> displayLocation);
    minpos  = _FirstWrapLine(wrapTable);
    maxpos  = _LastDisplayedWrapLine(wrapTable, page);

    switch (direction_amount) {
    case OL_SCROLLUP:
	new = _IncrementWrapLocation(wrapTable, current,    -1, minpos, &diff);
	break;
    case OL_SCROLLDOWN:
	new = _IncrementWrapLocation(wrapTable, current,     1, maxpos, &diff);
	break;
    case OL_PAGEUP:
	new = _IncrementWrapLocation(wrapTable, current, -page, minpos, &diff);
	break;
    case OL_PAGEDOWN:
	new = _IncrementWrapLocation(wrapTable, current,  page, maxpos, &diff);
	break;
    case OL_HOME:
	delta = -(text->displayPosition);
	new = _IncrementWrapLocByTextDelta(ctx, current, &delta, &diff, FALSE);
	break;
    case OL_END:
	if (text->text_format == OL_SB_STR_REP)
	    delta = LastTextBufferPosition(text->textBuffer);
	else
	    delta = OlLastTextBufferPosition((OlTextBufferPtr)
						text->textBuffer);
	delta -= text->displayPosition;
	new    = _IncrementWrapLocByTextDelta(ctx, current, &delta, &diff, FALSE);
	break;
    case OL_PGM_GOTO:
	diff = newpos;
	new = _IncrementWrapLocation(wrapTable, current, diff,
				     ((diff < 0) ? minpos : maxpos), &diff);
	break;
    default:
	new = current;
	OlWarning(dgettext(OlMsgsDomain, "TextEdit: Default in _MoveDisplayPosition."));
	break;
    }

    _MoveDisplay(ctx, text, text-> lineHeight, page, new, diff, event != NULL);
    return (diff);

} /* end of _MoveDisplayPosition */

/*
 * _MoveDisplay
 *
 * The \fI_MoveDisplay\fR procedure is used to move the text displayed
 * in the TextEdit widget \fIctx\fR to a \fInew\fR wrap location which
 * is known to be \fIdiff\fR wrap lines from the current wrap location.
 *
 * See also:
 *
 * _MoveDisplayLaterally(3)
 *
 * Synopsis:
 *
 *#include <TextPos.h>
 * ...
 */
extern void
_MoveDisplay(TextEditWidget ctx, TextEditPart *text, int fontht, int page, TextLocation new, int diff, int setSB)
{
Display *  display        = XtDisplay(ctx);
Window     window         = XtWindow(ctx);
int        absdiff        = abs(diff);
XEvent     expose_event;
XRectangle rect;

if (diff != 0)
   {
   _TurnTextCursorOff(ctx);
   _SetDisplayLocation(ctx, text, diff, setSB, new);

   if (!(text-> updateState && XtIsRealized((Widget)ctx)))
      return;

   rect.x      = 0;
   rect.width  = ctx-> core.width;

   rect.y      = PAGE_T_MARGIN(ctx);
   rect.height = PAGEHT(ctx);

   if (absdiff < page)
      {
      page = (page + 0) * fontht;
      rect.height = absdiff * fontht;
      if (diff < 0)
         {
         XCopyArea(display, window, window, text-> gc, 
           rect.x, rect.y, rect.width, page - rect.height,
           rect.x, rect.y + rect.height);
         }
      else
         {
         XCopyArea(display, window, window, text-> gc, 
           rect.x, rect.y + rect.height, rect.width, page - rect.height,
           rect.x, rect.y);
         rect.y = PAGE_T_MARGIN(ctx) + page - rect.height;
         }
      do 
         {
         XIfEvent(display, &expose_event, _IsGraphicsExpose, (char *)window);
         (*ctx-> core.widget_class-> core_class.expose)((Widget)ctx, &expose_event, NULL);
         } while (expose_event.type != NoExpose &&
                ((XGraphicsExposeEvent *)&expose_event)->count != 0);
      }
   _TurnTextCursorOff(ctx);
   XClearArea(display, window, rect.x, rect.y, rect.width, rect.height, False);
   _DisplayText(ctx, &rect);
   }

} /* end of _MoveDisplay */

/*
 * _MoveDisplayLaterally
 *
 */
extern void
_MoveDisplayLaterally(TextEditWidget ctx, TextEditPart *text, int diff, int setSB)
{
Display *      display = XtDisplay(ctx);
Window         window  = XtWindow(ctx);
int            absdiff = abs(diff);
XEvent         expose_event;
XRectangle     rect;
int            page;

if (diff != 0)
   {
   _TurnTextCursorOff(ctx);
   (void) _SetTextXOffset(ctx, text, diff, setSB);

   if (!(text-> updateState && XtIsRealized((Widget)ctx)))
      return;

   rect.x      = PAGE_L_MARGIN(ctx);
   page = rect.width  = PAGEWID(ctx);
   rect.y      = 0;
   rect.height = ctx-> core.height;

   if (absdiff < page)
      {
      page = rect.width;
      rect.width = absdiff;
      if (diff < 0)
         {
         XCopyArea(display, window, window, text-> gc, 
           rect.x, rect.y, page - rect.width, rect.height,
           rect.x + rect.width, rect.y);
         }
      else
         {
         XCopyArea(display, window, window, text-> gc, 
           rect.x + rect.width, rect.y, page - rect.width, rect.height,
           rect.x, rect.y);
         rect.x = PAGE_L_MARGIN(ctx) + page - rect.width;
         }
      do 
         {
         XIfEvent(display, &expose_event, _IsGraphicsExpose, (char *)window);
         (*ctx-> core.widget_class-> core_class.expose)((Widget)ctx, &expose_event, NULL);
         } while (expose_event.type != NoExpose &&
                ((XGraphicsExposeEvent *)&expose_event)->count != 0);
      }
   rect.x -= FONTWID(ctx);
   rect.x = MAX(rect.x, PAGE_L_MARGIN(ctx));
   rect.width += (2*FONTWID(ctx));  /* to refill any partially cleared chars */
   _TurnTextCursorOff(ctx);
   XClearArea(display, window, rect.x, rect.y, rect.width, rect.height, False);
   _DisplayText(ctx, &rect);
   }

} /* end of _MoveDisplayLaterally */

/*
 * _MoveCursorPosition
 *
 */
extern void
_MoveCursorPosition(TextEditWidget ctx,
		    XEvent *       event,
		    OlInputEvent   direction_amount,
		    TextPosition   newpos)
{
    if (ctx->textedit.text_format == OL_SB_STR_REP)
	_MoveCursorPositionSB(ctx, event, direction_amount, newpos);
    else if (!OlIsTextBufferEmpty(OlTextEditOlTextBuffer(ctx)))
	_MoveCursorPositionMBWC(ctx, event, direction_amount, newpos);
}

/*
 * _MoveCursorPositionSB
 *
 */
static void
_MoveCursorPositionSB(TextEditWidget ctx,
		      XEvent *       event,
		      OlInputEvent   direction_amount,
		      TextPosition   newpos)
{
    TextEditPart * text       = &(ctx-> textedit);
    WrapTable *    wrapTable  = text-> wrapTable;
    TextBuffer *   textBuffer = text-> textBuffer;
    TextLocation   current;
    TextLocation   new;
    int            diff;

    new = current = text-> cursorLocation;

    switch (direction_amount) {
    case OL_ROWUP:
    case OL_ROWDOWN:
	new = _WrapLocationOfLocation(wrapTable, new);
	if (text-> save_offset < 0)
	    text-> save_offset = current.offset - _WrapLineOffset(wrapTable,new);
	new = (direction_amount == OL_ROWUP) ?
	    _IncrementWrapLocation(wrapTable,new,-1,_FirstWrapLine(wrapTable),&diff):
	    _IncrementWrapLocation(wrapTable,new, 1,_LastWrapLine(wrapTable), &diff);
	if (diff == 0)
	    new = current;
	else {
	    diff = _WrapLineOffset(wrapTable, new) + text-> save_offset;
	    if (diff > _WrapLineEnd(textBuffer, wrapTable, new))
		new.offset = _WrapLineEnd(textBuffer, wrapTable, new);
	    else {
		new.offset = diff;
		text-> save_offset = -1;
            }
	}
	break;
    case OL_CHARFWD:
	new = IncrementTextBufferLocation(textBuffer, current,  0,  1);   break;
    case OL_CHARBAK:
	new = IncrementTextBufferLocation(textBuffer, current,  0, -1);   break;
    case OL_WORDFWD:
	new = NextTextBufferWord(textBuffer, current);                    break;
    case OL_WORDBAK:
	new = PreviousTextBufferWord(textBuffer, current);                break;
    case OL_LINESTART:
	new = _WrapLocationOfLocation(wrapTable, current);
	new.offset = 0;                                                   break;
    case OL_LINEEND:
	new = _WrapLocationOfLocation(wrapTable, current);
	new.offset = LastCharacterInTextBufferLine(textBuffer, new.line); break;
    case OL_DOCSTART:
	new.line = new.offset = 0;                                        break;
    case OL_DOCEND:
	new.line = LastTextBufferLine(textBuffer);
	new.offset = LastCharacterInTextBufferLine(textBuffer, new.line); break;
    case OL_PANESTART:
	new = _WrapLocationOfLocation(wrapTable, text-> displayLocation);
	new.offset = _WrapLineOffset(wrapTable, new);                     break;
    case OL_PANEEND:
	new = _WrapLocationOfLocation(wrapTable, text-> displayLocation);
	new = _IncrementWrapLocation(wrapTable, new, LINES_VISIBLE(ctx) - 1, 
		     _LastDisplayedWrapLine(wrapTable, LINES_VISIBLE(ctx)), NULL);
	new.offset = _WrapLineEnd(textBuffer, wrapTable, new);            break;
    default:
	OlWarning(dgettext(OlMsgsDomain, "TextEdit: Default in _MoveCursorPositionSB."));
    }
    if (direction_amount != OL_ROWUP && direction_amount != OL_ROWDOWN) 
	text-> save_offset = -1;

    (void)_MoveSelection(ctx, PositionOfLocation(textBuffer, new), 0, 0, 0);
}

/*
 * _MoveCursorPositionMBWC
 *
 */
static void
_MoveCursorPositionMBWC(TextEditWidget ctx,
			XEvent *       event,
			OlInputEvent   direction_amount,
			TextPosition   newpos)
{
    TextEditPart *    text       = &(ctx-> textedit);
    WrapTable *       wrapTable  = text-> wrapTable;
    OlTextBufferPtr   textBuffer = (OlTextBufferPtr)text-> textBuffer;
    TextLocation      current;
    TextLocation      new;
    int               diff;

    new = current = text-> cursorLocation;

    switch (direction_amount) {
    case OL_ROWUP:
    case OL_ROWDOWN:
	new = _WrapLocationOfLocation(wrapTable, new);
	if (text-> save_offset < 0)
	    text-> save_offset = current.offset - _WrapLineOffset(wrapTable,new);
	new = (direction_amount == OL_ROWUP) ?
	    _IncrementWrapLocation(wrapTable,new,-1,_FirstWrapLine(wrapTable),&diff):
		_IncrementWrapLocation(wrapTable,new, 1,_LastWrapLine(wrapTable), &diff);
	if (diff == 0)
	    new = current;
	else {
	    diff = _WrapLineOffset(wrapTable, new) + text-> save_offset;
	    if (diff > _WrapLineEnd((TextBuffer *)textBuffer, wrapTable, new))
		new.offset = _WrapLineEnd((TextBuffer *)textBuffer, wrapTable, new);
	    else {
		new.offset = diff;
		text-> save_offset = -1;
            }
	}
	break;
    case OL_CHARFWD:
	OlIncrementTextBufferLocation(textBuffer, &new,  0,  1);
	break;
    case OL_CHARBAK:
	OlIncrementTextBufferLocation(textBuffer, &new,  0, -1);
	break;
    case OL_WORDFWD:
	OlNextTextBufferWord(textBuffer, &new);
	break;
    case OL_WORDBAK:
	OlPreviousTextBufferWord(textBuffer, &new);
	break;
    case OL_LINESTART:
	new = _WrapLocationOfLocation(wrapTable, current);
	new.offset = 0;
	break;
    case OL_LINEEND:
	new = _WrapLocationOfLocation(wrapTable, current);
	new.offset = OlLastCharInTextBufferLine(textBuffer, new.line);
	break;
    case OL_DOCSTART:
	new.line = new.offset = 0;
	break;
    case OL_DOCEND:
	new.line = OlLastTextBufferLine(textBuffer);
	new.offset = OlLastCharInTextBufferLine(textBuffer, new.line);
	break;
    case OL_PANESTART:
	new = _WrapLocationOfLocation(wrapTable, text-> displayLocation);
	new.offset = _WrapLineOffset(wrapTable, new);
	break;
    case OL_PANEEND:
	new = _WrapLocationOfLocation(wrapTable, text-> displayLocation);
	new = _IncrementWrapLocation(wrapTable, new, LINES_VISIBLE(ctx) - 1, 
		     _LastDisplayedWrapLine(wrapTable, LINES_VISIBLE(ctx)), NULL);
	new.offset = _WrapLineEnd((TextBuffer *)textBuffer, wrapTable, new);
	break;
    default:
	OlWarning(dgettext(OlMsgsDomain, "TextEdit: Default in _MoveCursorPositionMBWC."));
    }
    if (direction_amount != OL_ROWUP && direction_amount != OL_ROWDOWN) 
	text-> save_offset = -1;

    (void)_MoveSelection(ctx, OlPositionOfLocation(textBuffer, &new), 0, 0, 0);
} /* end of _MoveCursorPositionMBWC */

/*
 * _MoveCursorPositionGlyph
 *
 */

extern void
_MoveCursorPositionGlyph(TextEditWidget ctx,
			 TextLocation   new)
{
    TextEditPart * text          = &(ctx->textedit);
    WrapTable *    wrapTable     = text->wrapTable;
    int            fontht        = text->lineHeight;
    int            page          = LINES_VISIBLE(ctx);
    TextLocation   currentIP;
    TextLocation   currentDP;
    int            row;
    int            xoffset;
    int		   delta;
    TextLocation   dummy;

    _TurnTextCursorOff(ctx);
    text-> cursorLocation = new;
    
    currentDP = _WrapLocationOfLocation(wrapTable, text-> displayLocation);
    if (wrapTable->text_format == OL_SB_STR_REP)
	delta = PositionOfLocation(text->textBuffer, new);
    else
	delta = OlPositionOfLocation((OlTextBufferPtr)text->textBuffer, &new);
    delta -= text->displayPosition;
    currentIP = _IncrementWrapLocByTextDelta(ctx, currentDP,
					     &delta, NULL, FALSE);
    dummy.line = dummy.offset = -1;
    _IncrementWrapLocation(wrapTable, currentIP, text->linesVisible,
			   dummy, NULL);
    _CalculateCursorRowAndXOffset(ctx, &row, &xoffset, currentDP, currentIP);

    _TurnTextCursorOff(ctx);

    if (_DrawTextCursor(ctx, row, currentIP, new.offset, ACTIVE_CARET(ctx)) == 0) {
	TextLocation dummy;

	dummy.line = -1;
	dummy.offset = -1;
	currentDP = (row >= page) ?
	    _IncrementWrapLocation(wrapTable, currentDP, row-(page-1), dummy, &row):
		_IncrementWrapLocation(wrapTable, currentDP, row, dummy, &row);
	_MoveDisplay(ctx, text, fontht, page, currentDP, row, TRUE);
    }

} /* end of _MoveCursorPositionGlyph */

/*
 * _ExtendSelection
 *
 */
extern void
_ExtendSelection(TextEditWidget ctx,
		 OlInputEvent   ol_event)
{
    if (ctx->textedit.text_format == OL_SB_STR_REP)
	_ExtendSelectionSB(ctx, ol_event);
    else
	_ExtendSelectionMBWC(ctx, ol_event);
}

/*
 * _ExtendSelectionSB
 *
 */
static void
_ExtendSelectionSB(TextEditWidget ctx,
		   OlInputEvent   ol_event)
{
    TextEditPart * text           = &ctx-> textedit;
    TextBuffer *   textBuffer     = text-> textBuffer;
    TextPosition   cursorPosition = text-> cursorPosition;
    TextPosition   selectStart    = text-> selectStart;
    TextPosition   selectEnd      = text-> selectEnd;
    TextLocation   x;
    TextLocation   newloc;
    TextLocation   oldloc;
    TextPosition   newpos;

    switch (ol_event) {
    case OL_SELCHARFWD:
	if (cursorPosition == selectEnd)
	    cursorPosition = ++selectEnd;
	else
	    cursorPosition = ++selectStart;
	break;
    case OL_SELCHARBAK:
	if (cursorPosition == selectStart)
	    cursorPosition = --selectStart;
	else
	    cursorPosition = --selectEnd;
	break;
    case OL_SELWORDFWD:
	oldloc = LocationOfPosition(textBuffer, selectEnd);
	newloc = EndCurrentTextBufferWord(textBuffer, oldloc);
	if (SameTextLocation(newloc, oldloc)) {
	    newloc = NextTextBufferWord(textBuffer, newloc);
	    newloc = EndCurrentTextBufferWord(textBuffer, newloc);
	}
	newpos = PositionOfLocation(textBuffer, newloc);
	if (newpos > selectEnd)
	    cursorPosition = selectEnd = PositionOfLocation(textBuffer, newloc);
	break;
    case OL_SELWORDBAK:
	oldloc = LocationOfPosition(textBuffer, selectStart);
	newloc = StartCurrentTextBufferWord(textBuffer, oldloc);
	if (SameTextLocation(newloc, oldloc))
	    newloc = PreviousTextBufferWord(textBuffer, newloc);
	newpos = PositionOfLocation(textBuffer, newloc);
	if (newpos < selectStart)
	    cursorPosition = selectStart = PositionOfLocation(textBuffer, newloc);
	break;
    case OL_SELLINEFWD:
	x = LocationOfPosition(textBuffer, selectEnd);
	if (x.offset == LastCharacterInTextBufferLine(textBuffer, x.line) && 
	    x.line != LastTextBufferLine(textBuffer))
	    x.line++;
	x.offset = LastCharacterInTextBufferLine(textBuffer, x.line);
	cursorPosition = 
	    selectEnd = PositionOfLocation(textBuffer, x);
	break;
    case OL_SELLINEBAK:
	x = LocationOfPosition(textBuffer, selectStart);
	if (x.offset == 0 && x.line != 0)
	    x.line--;
	x.offset = 0;
	cursorPosition = 
	    selectStart = PositionOfLocation(textBuffer, x);
	break;
    case OL_SELLINE:
	x = LocationOfPosition(textBuffer, selectStart);
	x.offset = 0;
	selectStart = PositionOfLocation(textBuffer, x);
	x = LocationOfPosition(textBuffer, selectEnd);
	x.offset = LastCharacterInTextBufferLine(textBuffer, x.line);
	cursorPosition = 
	    selectEnd = PositionOfLocation(textBuffer, x);
	newpos = text-> selectEnd + 1;
	if (newpos <= LastTextBufferPosition(textBuffer))
	    cursorPosition = text-> selectEnd = newpos;
	break;
    default:
	break;
    }
    if (0 <= cursorPosition && 
	cursorPosition <= LastTextBufferPosition(textBuffer))
	_MoveSelection(ctx, cursorPosition, selectStart, selectEnd, 7);
} /* end of _ExtendSelectionSB */

/*
 * _ExtendSelectionMBWC
 *
 */
static void
_ExtendSelectionMBWC(TextEditWidget ctx,
		     OlInputEvent   ol_event)
{
    TextEditPart *    text           = &ctx->textedit;
    OlTextBufferPtr   textBuffer     = (OlTextBufferPtr)text->textBuffer;
    TextPosition      cursorPosition = text->cursorPosition;
    TextPosition      selectStart    = text->selectStart;
    TextPosition      selectEnd      = text->selectEnd;
    TextLocation      x;
    TextLocation      newloc;
    TextLocation      oldloc;
    TextPosition      newpos;

    switch (ol_event) {
    case OL_SELCHARFWD:
	if (cursorPosition == selectEnd)
	    cursorPosition = ++selectEnd;
	else
	    cursorPosition = ++selectStart;
	break;
    case OL_SELCHARBAK:
	if (cursorPosition == selectStart)
	    cursorPosition = --selectStart;
	else
	    cursorPosition = --selectEnd;
	break;
    case OL_SELWORDFWD:
	OlLocationOfPosition(textBuffer, selectEnd, &oldloc);
	newloc = oldloc;
	OlEndCurrentTextBufferWord(textBuffer, &newloc);
	if (SameTextLocation(newloc, oldloc)) {
	    OlNextTextBufferWord(textBuffer, &newloc);
	    OlEndCurrentTextBufferWord(textBuffer, &newloc);
	}
	newpos = OlPositionOfLocation(textBuffer, &newloc);
	if (newpos > selectEnd)
	    cursorPosition = selectEnd = newpos;
	break;
    case OL_SELWORDBAK:
	OlLocationOfPosition(textBuffer, selectStart, &oldloc);
	newloc = oldloc;
	OlStartCurrentTextBufferWord(textBuffer, &newloc);
	if (SameTextLocation(newloc, oldloc))
	    OlPreviousTextBufferWord(textBuffer, &newloc);
	newpos = OlPositionOfLocation(textBuffer, &newloc);
	if (newpos < selectStart)
	    cursorPosition = selectStart = newpos;
	break;
    case OL_SELLINEFWD:
	OlLocationOfPosition(textBuffer, selectEnd, &x);
	if (x.offset == OlLastCharInTextBufferLine(textBuffer, x.line) && 
	    x.line   != OlLastTextBufferLine(textBuffer))
	    x.line++;
	x.offset       = OlLastCharInTextBufferLine(textBuffer, x.line);
	cursorPosition = 
	    selectEnd      = OlPositionOfLocation(textBuffer, &x);
	break;
    case OL_SELLINEBAK:
	OlLocationOfPosition(textBuffer, selectStart, &x);
	if (x.offset == 0 && x.line != 0)
	    x.line--;
	x.offset       = 0;
	cursorPosition = 
	    selectStart    = OlPositionOfLocation(textBuffer, &x);
	break;
    case OL_SELLINE:
	OlLocationOfPosition(textBuffer, selectStart, &x);
	x.offset       = 0;
	selectStart    = OlPositionOfLocation(textBuffer, &x);
	OlLocationOfPosition(textBuffer, selectEnd, &x);
	x.offset       = OlLastCharInTextBufferLine(textBuffer, x.line);
	cursorPosition = 
	selectEnd      = OlPositionOfLocation(textBuffer, &x);
	newpos         = text-> selectEnd + 1;
	if (newpos <= OlLastTextBufferPosition(textBuffer))
	    cursorPosition = text-> selectEnd = newpos;
	break;
    default:
	break;
    }
    if (0 <= cursorPosition && 
	cursorPosition <= OlLastTextBufferPosition(textBuffer))
	_MoveSelection(ctx, cursorPosition, selectStart, selectEnd, 7);
} /* end of _ExtendSelectionMBWC */

/*
 * _MoveSelection
 *
 * The \fI_MoveSelection\fR function performs the actual movement of
 * the selection and cursor point in the TextEdit widget \fIctx\fR.
 * 
 * See also:
 *
 * _MoveCursorPosition(3), TextSetCursorPosition(3)
 *
 * Synopsis:
 *
 *#include <TextPos.h>
 * ...
 */
extern Boolean
_MoveSelection(TextEditWidget ctx,
	       TextPosition   position,
	       TextPosition   selectStart,
	       TextPosition   selectEnd,
	       int            selectMode)
{
    if (ctx->textedit.text_format == OL_SB_STR_REP)
	return(_MoveSelectionSB(ctx, position, selectStart, selectEnd, selectMode));
    else
	return(_MoveSelectionMBWC(ctx, position, selectStart, selectEnd, selectMode));
}

/*
 * _MoveSelectionSB
 *
 */
static Boolean
_MoveSelectionSB(TextEditWidget ctx,
		 TextPosition   position,
		 TextPosition   selectStart,
		 TextPosition   selectEnd,
		 int            selectMode)
{
    TextEditPart * text       = &ctx-> textedit;
    TextBuffer *   textBuffer = text-> textBuffer;
    XRectangle     rect;

    OlTextMotionCallData call_data;

    TextLocation   start;
    TextLocation   end;
    TextLocation   new_loc;
    TextLocation   temploc;
    TextPosition   new_start;
    TextPosition   new_end;
    TextPosition   new_pos;
    TextPosition   mid_pos;
    TextPosition   temppos;

    int            i;		/* @ TextPosition i; */
    TextLine       j;
    char *         p;
    TextLocation   newEnd;

    start = end = LocationOfPosition(textBuffer, position);
    switch(text-> selectMode = selectMode) {
    case 0:			/* char */
	text-> anchor = -1;
	break;
    case 1:			/* word */
	if(text->anchor == -1)
	    text-> anchor = PositionOfLocation(textBuffer,
					       StartCurrentTextBufferWord(textBuffer, start));
	start = StartCurrentTextBufferWord(textBuffer, start);
	end   = EndCurrentTextBufferWord(textBuffer, start);
	break;
    case 2:			/* line */
	if(text->anchor == -1) {
	    temploc = LocationOfPosition(textBuffer,position);
	    temploc.offset = 0;
	    text->anchor = PositionOfLocation(textBuffer,temploc);
	}
	start.offset = 0;
	end.offset   = LastCharacterInTextBufferLine(textBuffer, end.line);
	break;
    case 3:			/* paragraph */
	for (j = start.line; j >= 0; j--)
	    if (LastCharacterInTextBufferLine(textBuffer, j) == 0)
		break;
	start.line   = MIN(j + 1, LastTextBufferLine(textBuffer));
	start.offset = 0;
	for (j = end.line; j <= LastTextBufferLine(textBuffer); j++)
	    if (LastCharacterInTextBufferLine(textBuffer, j) == 0)
		break;
	end.line     = MAX(j - 1, 0);
	end.offset   = LastCharacterInTextBufferLine(textBuffer, end.line);
	break;
    case 4:			/* document  */
	start.line   = 0;
	start.offset = 0;
	end.line     = LastTextBufferLine(textBuffer);
	end.offset   = LastCharacterInTextBufferLine(textBuffer, end.line);
	break;
    case 5:			/* adjust click: char mode */
	if(text-> anchor == -1) {
	    text-> anchor = text->cursorPosition;
	    if(position <= text-> anchor) {
		end = LocationOfPosition(textBuffer,text->anchor);
		text-> anchor = position;
	    } else 
		start = LocationOfPosition(textBuffer,text->anchor);
	} else {
	    if ( (position <= text->selectEnd-1) && 
		(position >= text-> selectStart) )
	    {		/*On sel*/
		start = LocationOfPosition(textBuffer,
					   text->selectStart);
	    } else  {	/* Off selection */
		if(position <= text-> anchor) {
		    end = LocationOfPosition(textBuffer,
					     text->selectEnd);
		    text->anchor = position;
		} else {
		    start = LocationOfPosition(textBuffer,
					       text-> selectStart);
		}
	    }		/* end Off selection */
	} 
	end = _NextLocationWithoutWrap(textBuffer,end);
	break;
    case 51:			/* adjust click: word mode */
	if(text-> anchor == -1) {
	    text-> anchor = text-> selectStart;
	    if(position <= text-> anchor) {
		end = LocationOfPosition(textBuffer, text->selectEnd);
		temploc = LocationOfPosition(textBuffer, position);  
		start = StartCurrentTextBufferWord(textBuffer, temploc);
		text-> anchor = PositionOfLocation(textBuffer, start);
	    } else  {
		start = LocationOfPosition(textBuffer, text->selectStart);
		temploc = LocationOfPosition(textBuffer, position);  
		end = EndCurrentTextBufferWord(textBuffer, temploc);
	    }
	} else {
	    if ((position >= text-> selectStart) &&
		(position <= text-> selectEnd-1))
	    {		/* On sel */
		start = LocationOfPosition(textBuffer, text-> selectStart);
		end   =  EndCurrentTextBufferWord(textBuffer,
						  LocationOfPosition(textBuffer, 
								     position));
	    } else  {	/* Off selection */
		if(position <= text-> anchor) {
		    end = LocationOfPosition(textBuffer, text-> selectEnd);
		    start = StartCurrentTextBufferWord(textBuffer,
						       LocationOfPosition(textBuffer, 
									  position));
		    text-> anchor = PositionOfLocation(textBuffer, start);
		} else {
		    start = LocationOfPosition(textBuffer, text-> selectStart);
		    end = EndCurrentTextBufferWord(textBuffer,
						   LocationOfPosition(textBuffer, 
								      position));
		}
	    }
	}
	break;
    case 52:			/* adjust click: line mode */
	if(text-> anchor == -1) {
	    text-> anchor = text-> selectStart;
	    if(position <= text-> anchor) {
		end = LocationOfPosition(textBuffer, text->selectEnd);
		start = LocationOfPosition(textBuffer, position);  
		start.offset = 0;	
		text-> anchor = PositionOfLocation(textBuffer, start);
	    } else  {
		start = LocationOfPosition(textBuffer, text->selectStart);
		end = LocationOfPosition(textBuffer, position);  
		end.offset = LastCharacterInTextBufferLine(textBuffer, end.line);
	    }
	} else {
	    if ( (position >= text-> selectStart) &&
		(position <= text-> selectEnd-1) ) { /* On sel */
		start = LocationOfPosition(textBuffer, text-> selectStart);
		end = LocationOfPosition(textBuffer, position);  
		end.offset = LastCharacterInTextBufferLine(textBuffer, end.line);
	    } else  {		/* Off selection */
		if(position <= text-> anchor) {
		    end = LocationOfPosition(textBuffer, text-> selectEnd);
		    start   = 
			LocationOfPosition(textBuffer, position);
		    start.offset = 0;
		    text-> anchor = 
			PositionOfLocation(textBuffer, start);
		} else {
		    start = LocationOfPosition(textBuffer, text-> selectStart);
		    end = LocationOfPosition(textBuffer, position);  
		    end.offset = LastCharacterInTextBufferLine(textBuffer, end.line);
		}
	    }
		
	}
	break;
    case 6:			/* select/adjust sweep poll */
	if (text-> anchor == -1)
	    text-> anchor = text-> cursorPosition;
	if (position <= text-> anchor)
	    end = LocationOfPosition(textBuffer, text-> anchor);
	else {
	    TextLocation newEnd;
	    start = LocationOfPosition(textBuffer, text-> anchor);
	    newEnd = NextLocation(textBuffer,end);	
	    /* fix for bug 1111332 */
	    if (!(newEnd.line == 0 && newEnd.offset == 0 )) 
	    {
		/* Not at the end of text */
		end = newEnd;
	    }
		
	}
	break;
    case 65:			/* adjust poll: char mode */
	if(position <= text-> anchor) {
	    end = LocationOfPosition(textBuffer, text->selectEnd);
	} else {
	    start = LocationOfPosition(textBuffer, text-> anchor);
	}
	break;
    case 85:			/* adjust  poll: word mode*/
	if(position <= text-> anchor) {
	    end = LocationOfPosition(textBuffer, text-> selectEnd);
	    start   =  
		StartCurrentTextBufferWord(textBuffer, 
					   LocationOfPosition(textBuffer, position));
	} else {
	    start = LocationOfPosition(textBuffer, text-> anchor);
	    end = EndCurrentTextBufferWord(textBuffer,
					   LocationOfPosition(textBuffer, position));
	}
	break;
    case 95:			/* adjust poll: line mode  */
	if(position <= text-> anchor) {
	    end = LocationOfPosition(textBuffer, text-> selectEnd);
	    start = LocationOfPosition(textBuffer, position);
	    start.offset = 0;
	} else {
	    start = LocationOfPosition(textBuffer, text-> anchor);
	    start.offset = 0;
	    end = LocationOfPosition(textBuffer, position);  
	    end.offset = LastCharacterInTextBufferLine(textBuffer, end.line);
	}
	break;
    case 7:			/* programatic go to */
	start = LocationOfPosition(textBuffer, selectStart);
	end = LocationOfPosition(textBuffer, selectEnd);
	break;
    case 8:			/* double SELECT click sweep poll at word granularity */
	if(text->anchor == -1)
	    text-> anchor = PositionOfLocation(textBuffer,
					       StartCurrentTextBufferWord(textBuffer, start));
	if (position < text-> anchor) {
	    temploc = LocationOfPosition(textBuffer, text-> anchor);
	    end =  EndCurrentTextBufferWord(textBuffer,temploc);
	    start =  StartCurrentTextBufferWord(textBuffer,
						LocationOfPosition(textBuffer, position));
	} else {
	    start = LocationOfPosition(textBuffer, text-> anchor);
	    end   = EndCurrentTextBufferWord(textBuffer,
					     LocationOfPosition(textBuffer, position));
	}
	break;
    case 9:			/* triple SELECT click sweep poll at line granularity */
	if(text->anchor == -1) {
	    temploc = LocationOfPosition(textBuffer,position);
	    temploc.offset = 0;
	    text->anchor = PositionOfLocation(textBuffer,temploc);
	}

	if(position < text->anchor) {
	    end = LocationOfPosition(textBuffer,text->anchor);
	    end.offset = LastCharacterInTextBufferLine(textBuffer,end.line);
	    start = LocationOfPosition(textBuffer,position);
	    start.offset = 0;
	} else  {
	    start = LocationOfPosition(textBuffer,text->anchor);
	    start.offset = 0;
	    end = LocationOfPosition(textBuffer,position);
	    end.offset = LastCharacterInTextBufferLine(textBuffer,end.line);
	}
	break;
    }
    new_start = PositionOfLocation(textBuffer, start);
    new_end   = PositionOfLocation(textBuffer, end);
    new_pos = text-> cursorPosition;

    if ((text-> selectMode == 6) ||
	(text-> selectMode == 8) ||
	(text-> selectMode == 9) ||
	(text-> selectMode == 65) ||
	(text-> selectMode == 85) ||
	(text-> selectMode == 95))
    {
	if (position <= text-> anchor) {
	    if (text-> cursorPosition != new_start) {
		new_pos = new_start;
		new_loc = start;
	    }
	} else {
	    if (text-> cursorPosition != new_end) {
		new_pos = new_end;
		new_loc = end;
	    }
	}
    } else if ((text-> selectMode == 5) && (text->selectMode == 51)
	       && (text->selectMode == 52) && position <= text-> selectStart)
    {
	if (text-> cursorPosition != new_start) {
	    new_pos = new_start;
	    new_loc = start;
	}
    } else if (text-> selectMode == 7) {
	new_pos = position;
	new_loc = LocationOfPosition(textBuffer, position);
    } else {
	new_pos = new_end;
	new_loc = end;
    }

    call_data.ok = True;
    if (text-> cursorPosition != new_pos) {
	call_data.current_cursor = text-> cursorPosition;
	call_data.new_cursor     = new_pos;
	call_data.select_start   = new_start;
	call_data.select_end     = new_end;
	XtCallCallbacks((Widget)ctx, XtNmotionVerification, &call_data);
    }

    if (call_data.ok) {
#ifdef CALLBACK_CAN_RESET_MOTION_VALUES
	new_pos   = call_data.new_cursor;
	new_start = call_data.select_start;
	new_end   = call_data.select_end;
#endif
	if (new_pos != text-> cursorPosition) {
	    text-> cursorPosition = new_pos;
	    new_loc   = LocationOfPosition(textBuffer, new_pos);

	    /* fix for bug 1128853
	     * programmatically moving cursor to a new location should
	     * display that location. This used to work in 3.1 but
	     * was broken in 3.1 
	     */
	    if ( text->selectMode == 7 ) {
		TextLocation	currentIP, currentDP;
		int		row, xoffset;
		TextLocation	temp;

		temp = text->cursorLocation;
		text->cursorLocation = new_loc;
		_TurnTextCursorOff ( ctx );
		currentDP = _WrapLocationOfLocation( text->wrapTable, 
						     text->displayLocation );
		currentIP = _WrapLocationOfLocation( text->wrapTable, 
						     new_loc );

		_CalculateCursorRowAndXOffset( ctx, &row, &xoffset, currentDP,
						currentIP );
		if ( xoffset != 0 )
		    _MoveDisplayLaterally( ctx, text, xoffset, 
					   text->updateState );

		_TurnTextCursorOff ( ctx );
		text->cursorLocation = temp;
	    }
	    _MoveCursorPositionGlyph(ctx, new_loc);

	}

	if (new_start == text-> selectStart && new_end == text-> selectEnd)
	    ;			/* nop */
	else if (text-> selectStart == text-> selectEnd && new_start == new_end) {
	    text-> selectStart = new_start;
	    text-> selectEnd   = new_end;
	} else {
	    if (new_start == text-> selectStart)
		rect = (new_end < text-> selectEnd) ?
		    _RectFromPositions(ctx, new_end, text-> selectEnd)     :
	    _RectFromPositions(ctx, text-> selectEnd, new_end);
	    else if (new_end == text-> selectEnd)
		rect = (new_start < text-> selectStart) ?
		    _RectFromPositions(ctx, new_start, text-> selectStart):
			_RectFromPositions(ctx, text-> selectStart, new_start);
	    else {
		rect = _RectFromPositions(ctx, text-> selectStart, text-> selectEnd);
		text-> selectStart = text-> selectEnd   = -1;
		if (rect.width != 0 && rect.height != 0)
		    _DisplayText(ctx, &rect);
		rect = _RectFromPositions(ctx, new_start, new_end);
	    }
	    text-> selectStart = new_start;
	    text-> selectEnd   = new_end;
	    if (rect.width != 0 && rect.height != 0)
		_DisplayText(ctx, &rect);
	}
	if (text-> selectStart < text-> selectEnd &&
	    !XtOwnSelection((Widget)ctx, XA_PRIMARY, 
			    _XtLastTimestampProcessed((Widget)ctx), 
			    ConvertPrimary, LosePrimary, NULL))
 	{
	    OlWarning(dgettext(OlMsgsDomain,
			       "TextEdit: Didn't get the primary selection!"));
	    if (text-> selectStart == text-> cursorPosition)
		text-> selectEnd = text-> cursorPosition;
	    else
		text-> selectStart = text-> cursorPosition;
	}
    }

    return (call_data.ok);
} /* end of _MoveSelectionSB */

/*
 * _MoveSelectionMBWC
 *
 */
static Boolean
_MoveSelectionMBWC(TextEditWidget ctx,
		 TextPosition   position,
		 TextPosition   selectStart,
		 TextPosition   selectEnd,
		 int            selectMode)
{
    TextEditPart *       text       = &ctx-> textedit;
    OlTextBufferPtr      textBuffer = (OlTextBufferPtr)text-> textBuffer;
    XRectangle           rect;

    OlTextMotionCallData call_data;

    TextLocation         start;
    TextLocation         end;
    TextLocation         new_loc;
    TextLocation         temploc;
    TextPosition         new_start;
    TextPosition         new_end;
    TextPosition         new_pos;
    TextPosition         mid_pos;
    TextPosition         temppos;

    int                  i;		/* @ TextPosition i; */
    TextLine             j;
    char *               p;
    TextLocation	 newEnd;

    OlLocationOfPosition(textBuffer, position, &end);
    start = end;
    switch(text-> selectMode = selectMode) {
    case 0:			/* char */
	text-> anchor = -1;
	break;
    case 1:			/* word */
	if(text->anchor == -1)
	    text-> anchor = OlPositionOfLocation(textBuffer,
				       OlStartCurrentTextBufferWord(textBuffer, &start));
	OlStartCurrentTextBufferWord(textBuffer, &start);
	end = start;
	OlEndCurrentTextBufferWord(textBuffer, &end);
	break;
    case 2:			/* line */
	if(text->anchor == -1) {
	    OlLocationOfPosition(textBuffer,position, &temploc);
	    temploc.offset = 0;
	    text->anchor = OlPositionOfLocation(textBuffer, &temploc);
	}
	start.offset = 0;
	end.offset   = OlLastCharInTextBufferLine(textBuffer, end.line);
	break;
    case 3:			/* paragraph */
	for (j = start.line; j >= 0; j--)
	    if (OlLastCharInTextBufferLine(textBuffer, j) == 0)
		break;
	start.line   = MIN(j + 1, OlLastTextBufferLine(textBuffer));
	start.offset = 0;
	for (j = end.line; j <= OlLastTextBufferLine(textBuffer); j++)
	    if (OlLastCharInTextBufferLine(textBuffer, j) == 0)
		break;
	end.line     = MAX(j - 1, 0);
	end.offset   = OlLastCharInTextBufferLine(textBuffer, end.line);
	break;
    case 4:			/* document  */
	start.line   = 0;
	start.offset = 0;
	end.line     = OlLastTextBufferLine(textBuffer);
	end.offset   = OlLastCharInTextBufferLine(textBuffer, end.line);
	break;
    case 5:			/* adjust click: char mode */
	if(text-> anchor == -1) {
	    text-> anchor = text->cursorPosition;
	    if(position <= text-> anchor) {
		OlLocationOfPosition(textBuffer,text->anchor, &end);
		text-> anchor = position;
	    } else 
		OlLocationOfPosition(textBuffer,text->anchor, &start);
	} else {
	    if ((position <= text->selectEnd-1) && 
		(position >= text-> selectStart))
	    {		/*On sel*/
		OlLocationOfPosition(textBuffer, text->selectStart, &start);
	    } else  {	/* Off selection */
		if(position <= text-> anchor) {
		    OlLocationOfPosition(textBuffer, text->selectEnd, &end);
		    text->anchor = position;
		} else {
		    OlLocationOfPosition(textBuffer, text-> selectStart, &start);
		}
	    }		/* end Off selection */
	} 
	(void)_OlNextLocationWithoutWrap(textBuffer,&end);
	break;
    case 51:			/* adjust click: word mode */
	if(text-> anchor == -1) {
	    text-> anchor = text-> selectStart;
	    if(position <= text-> anchor) {
		OlLocationOfPosition(textBuffer, text->selectEnd, &end);
		OlLocationOfPosition(textBuffer, position, &temploc);
		start = temploc;
		OlStartCurrentTextBufferWord(textBuffer, &start);
		text-> anchor = OlPositionOfLocation(textBuffer, &start);
	    } else  {
		OlLocationOfPosition(textBuffer, text->selectStart, &start);
		OlLocationOfPosition(textBuffer, position, &temploc);
		end = temploc;
		OlEndCurrentTextBufferWord(textBuffer, &temploc);
	    }
	} else {
	    if ((position >= text-> selectStart) &&
		(position <= text-> selectEnd-1))
	    {		/* On sel */
		OlLocationOfPosition(textBuffer, text-> selectStart, &start);
		OlEndCurrentTextBufferWord(textBuffer,
					   OlLocationOfPosition(textBuffer, 
								position, &end));
	    } else  {	/* Off selection */
		if (position <= text-> anchor) {
		    OlLocationOfPosition(textBuffer, text-> selectEnd, &end);
		    OlStartCurrentTextBufferWord(textBuffer,
						 OlLocationOfPosition(textBuffer, 
								      position, &start));
		    text-> anchor = OlPositionOfLocation(textBuffer, &start);
		} else {
		    OlLocationOfPosition(textBuffer, text-> selectStart, &start);
		    OlEndCurrentTextBufferWord(textBuffer,
					       OlLocationOfPosition(textBuffer, position, &end));
		}
	    }
	}
	break;
    case 52:			/* adjust click: line mode */
	if(text-> anchor == -1) {
	    text-> anchor = text-> selectStart;
	    if(position <= text-> anchor) {
		OlLocationOfPosition(textBuffer, text->selectEnd, &end);
		OlLocationOfPosition(textBuffer, position, &start);  
		start.offset = 0;	
		text->anchor = OlPositionOfLocation(textBuffer, &start);
	    } else  {
		OlLocationOfPosition(textBuffer, text->selectStart, &start);
		OlLocationOfPosition(textBuffer, position, &end);  
		end.offset = OlLastCharInTextBufferLine(textBuffer, end.line);
	    }
	} else {
	    if ( (position >= text-> selectStart) &&
		(position <= text-> selectEnd-1) )
	    { /* On sel */
		OlLocationOfPosition(textBuffer, text-> selectStart, &start);
		OlLocationOfPosition(textBuffer, position, &end);  
		end.offset = OlLastCharInTextBufferLine(textBuffer, end.line);
	    } else  {		/* Off selection */
		if(position <= text-> anchor) {
		    OlLocationOfPosition(textBuffer, text-> selectEnd, &end);
		    OlLocationOfPosition(textBuffer, position, &start);
		    start.offset = 0;
		    text-> anchor = OlPositionOfLocation(textBuffer, &start);
		} else {
		    OlLocationOfPosition(textBuffer, text-> selectStart, &start);
		    OlLocationOfPosition(textBuffer, position, &end);  
		    end.offset = OlLastCharInTextBufferLine(textBuffer, end.line);
		}
	    }
	}
	break;
    case 6:			/* select/adjust sweep poll */
	if (text-> anchor == -1)
	    text-> anchor = text-> cursorPosition;
	if (position <= text-> anchor)
	    OlLocationOfPosition(textBuffer, text-> anchor, &end);
	else {
	    TextLocation newEnd = end;
	    OlLocationOfPosition(textBuffer, text-> anchor, &start);
	    OlNextLocation(textBuffer,&newEnd);
	    /* fix for bug 1111332 */
	    if (!(newEnd.line == 0 && newEnd.offset == 0 )) 
	    {
		/* Not at the end of text */
		end = newEnd;
	    }
	}
	break;
    case 65:			/* adjust poll: char mode */
	if(position <= text-> anchor) {
	    OlLocationOfPosition(textBuffer, text->selectEnd, &end);
	} else {
	    OlLocationOfPosition(textBuffer, text->anchor, &start);
	}
	break;
    case 85:			/* adjust  poll: word mode*/
	if(position <= text-> anchor) {
	    OlLocationOfPosition(textBuffer, text-> selectEnd, &end);
	    OlStartCurrentTextBufferWord(textBuffer, 
		   OlLocationOfPosition(textBuffer, position, &start));
	} else {
	    OlLocationOfPosition(textBuffer, text-> anchor, &start);
	    OlEndCurrentTextBufferWord(textBuffer,
			   OlLocationOfPosition(textBuffer, position, &end));
	}
	break;
    case 95:			/* adjust poll: line mode  */
	if(position <= text-> anchor) {
	    OlLocationOfPosition(textBuffer, text-> selectEnd, &end);
	    OlLocationOfPosition(textBuffer, position, &start);
	    start.offset = 0;
	} else {
	    OlLocationOfPosition(textBuffer, text-> anchor, &start);
	    start.offset = 0;
	    OlLocationOfPosition(textBuffer, position, &end);  
	    end.offset = OlLastCharInTextBufferLine(textBuffer, end.line);
	}
	break;
    case 7:			/* programatic go to */
	OlLocationOfPosition(textBuffer, selectStart, &start);
	OlLocationOfPosition(textBuffer, selectEnd, &end);
	break;
    case 8:			/* double SELECT click sweep poll at word granularity */
	if(text->anchor == -1) {
	    temploc = start;
	    text-> anchor = OlPositionOfLocation(textBuffer,
			       OlStartCurrentTextBufferWord(textBuffer, &temploc));
	}
	if (position < text-> anchor) {
	    OlLocationOfPosition(textBuffer, text-> anchor, &temploc);
	    end = temploc;
	    OlEndCurrentTextBufferWord(textBuffer, &end);
	    OlStartCurrentTextBufferWord(textBuffer,
			OlLocationOfPosition(textBuffer, position, &start));
	} else {
	    OlLocationOfPosition(textBuffer, text-> anchor, &start);
	    OlEndCurrentTextBufferWord(textBuffer,
		     OlLocationOfPosition(textBuffer, position, &end));
	}
	break;
    case 9:			/* triple SELECT click sweep poll at line granularity */
	if(text->anchor == -1) {
	    OlLocationOfPosition(textBuffer,position, &temploc);
	    temploc.offset = 0;
	    text->anchor = OlPositionOfLocation(textBuffer, &temploc);
	}
	if (position < text->anchor) {
	    OlLocationOfPosition(textBuffer,text->anchor, &end);
	    end.offset = OlLastCharInTextBufferLine(textBuffer, end.line);
	    OlLocationOfPosition(textBuffer,position, &start);
	    start.offset = 0;
	} else  {
	    OlLocationOfPosition(textBuffer,text->anchor, &start);
	    start.offset = 0;
	    OlLocationOfPosition(textBuffer,position, &end);
	    end.offset = OlLastCharInTextBufferLine(textBuffer, end.line);
	}
	break;
    }
 
    new_start = OlPositionOfLocation(textBuffer, &start);
    new_end   = OlPositionOfLocation(textBuffer, &end);
    new_pos   = text-> cursorPosition;

    if ((text-> selectMode == 6) ||
	(text-> selectMode == 8) ||
	(text-> selectMode == 9) ||
	(text-> selectMode == 65) ||
	(text-> selectMode == 85) ||
	(text-> selectMode == 95))
    {
	if (position <= text-> anchor) {
	    if (text-> cursorPosition != new_start) {
		new_pos = new_start;
		new_loc = start;
	    }
	} else {
	    if (text-> cursorPosition != new_end) {
		new_pos = new_end;
		new_loc = end;
	    }
	}
    } else if ((text-> selectMode == 5) && (text->selectMode == 51)
	       && (text->selectMode == 52) && position <= text-> selectStart)
    {
	if (text-> cursorPosition != new_start) {
	    new_pos = new_start;
	    new_loc = start;
	}
    } else if (text-> selectMode == 7) {
	new_pos = position;
	OlLocationOfPosition(textBuffer, position, &new_loc);
    } else {
	new_pos = new_end;
	new_loc = end;
    }

    call_data.ok = True;
    if (text-> cursorPosition != new_pos) {
	call_data.current_cursor = text-> cursorPosition;
	call_data.new_cursor     = new_pos;
	call_data.select_start   = new_start;
	call_data.select_end     = new_end;
	XtCallCallbacks((Widget)ctx, XtNmotionVerification, &call_data);
    }

    if (call_data.ok) {
#ifdef CALLBACK_CAN_RESET_MOTION_VALUES
	new_pos   = call_data.new_cursor;
	new_start = call_data.select_start;
	new_end   = call_data.select_end;
#endif
	if (new_pos != text-> cursorPosition) {
	    text-> cursorPosition = new_pos;
	    OlLocationOfPosition(textBuffer, new_pos, &new_loc);

	    /* fix for bug 1128853
	     * programmatically moving cursor to a new location should
	     * display that location. This used to work in 3.1 but
	     * was broken in 3.1 
	     */
	    if ( text->selectMode == 7 ) {
		TextLocation	currentIP, currentDP;
		int		row, xoffset;
		TextLocation	temp;

		temp = text->cursorLocation;
		text->cursorLocation = new_loc;
		_TurnTextCursorOff ( ctx );
		currentDP = _WrapLocationOfLocation( text->wrapTable, 
						     text->displayLocation );
		currentIP = _WrapLocationOfLocation( text->wrapTable, 
						     new_loc );

		_CalculateCursorRowAndXOffset( ctx, &row, &xoffset, currentDP,
						currentIP );
		if ( xoffset != 0 )
		    _MoveDisplayLaterally( ctx, text, xoffset, 
					   text->updateState );

		_TurnTextCursorOff ( ctx );
		text->cursorLocation = temp;
	    }
	    _MoveCursorPositionGlyph(ctx, new_loc);
	}

	if (new_start == text-> selectStart && new_end == text-> selectEnd)
	    ;			/* nop */
	else if (text-> selectStart == text-> selectEnd && new_start == new_end) {
	    text-> selectStart = new_start;
	    text-> selectEnd   = new_end;
	} else {
	    if (new_start == text-> selectStart)
		rect = (new_end < text-> selectEnd) ?
		    _RectFromPositions(ctx, new_end, text-> selectEnd)     :
	    _RectFromPositions(ctx, text-> selectEnd, new_end);
	    else if (new_end == text-> selectEnd)
		rect = (new_start < text-> selectStart) ?
		    _RectFromPositions(ctx, new_start, text-> selectStart):
			_RectFromPositions(ctx, text-> selectStart, new_start);
	    else {
		rect = _RectFromPositions(ctx, text-> selectStart, text-> selectEnd);
		text-> selectStart = text-> selectEnd   = -1;
		if (rect.width != 0 && rect.height != 0)
		    _DisplayText(ctx, &rect);
		rect = _RectFromPositions(ctx, new_start, new_end);
	    }
	    text-> selectStart = new_start;
	    text-> selectEnd   = new_end;
	    if (rect.width != 0 && rect.height != 0)
		_DisplayText(ctx, &rect);
	}
	if (text-> selectStart < text-> selectEnd &&
	    !XtOwnSelection((Widget)ctx, XA_PRIMARY, 
			    _XtLastTimestampProcessed((Widget)ctx), 
			    ConvertPrimary, LosePrimary, NULL))
 	{
	    OlWarning(dgettext(OlMsgsDomain,
			       "TextEdit: Didn't get the primary selection!"));
	    if (text-> selectStart == text-> cursorPosition)
		text-> selectEnd = text-> cursorPosition;
	    else
		text-> selectStart = text-> cursorPosition;
	}
    }
    return (call_data.ok);
} /* end of _MoveSelectionMBWC */

/*
 * _TextEditOwnPrimary
 *
 */

extern int
_TextEditOwnPrimary(TextEditWidget ctx, Time time)
{
int retval;

if (XtOwnSelection((Widget)ctx, XA_PRIMARY, time, ConvertPrimary, LosePrimary, NULL))
   retval = 1;
else
   retval = 0;

return (retval);

} /* end of _TextEditOwnPrimary */
/*
 * _PositionFromXY
 *
 * The \fI_PositionFromXY\fR function is used to calculate the text position
 * of the nearest character in the TextEdit widget \fIctx\fR for the 
 * coordinates \fIx\fR and \fIy\fR.
 *
 * See also:
 * 
 * _RectFromPositions(3)
 *
 * Synopsis:
 *
 *#include <TextPos.h>
 * ...
 */

extern TextPosition
_PositionFromXY(TextEditWidget ctx, int x, int y, PositionType type)
{
    TextEditPart * text       = &ctx-> textedit;
    TextBuffer *   textBuffer = text->textBuffer;
    WrapTable *    wrapTable  = text-> wrapTable;
    TabTable       tabs       = text-> tabs;
    int            fontht     = text-> lineHeight;
    int            start      = fontht ?
				(y - (int)PAGE_T_MARGIN(ctx)) / fontht : 0;
    int            startx     = PAGE_L_MARGIN(ctx) + text-> xOffset;
    int            diff;
    TextLocation   new;
    TextLocation   current;
    TextPosition   position;
    TextPosition   maxi;
    int            maxx;
    int		   lastx;
    
    register int            c;
    register TextPosition   i;
    OlStrRep	            rep = text->text_format;

    current = _WrapLocationOfLocation(wrapTable, text-> displayLocation);
    if (start != 0) {
	TextLocation	foo;
	if (start < 0)
	    foo = _FirstWrapLine(wrapTable);
	else
	    foo = _LastWrapLine(wrapTable);

	new = _IncrementWrapLocation(wrapTable, current, start, foo, NULL);
    } else
	new = current;

    maxi = _WrapLineEnd(textBuffer, wrapTable, new);
    maxx = x;
    new = _LocationOfWrapLocation(wrapTable, new);

    switch (rep) {
    case OL_SB_STR_REP:
	{
	    TextBuffer *    textBuffer = text-> textBuffer;
	    XFontStruct *   fs         = (XFontStruct *)ctx->primitive.font;
	    int             min        = fs->min_char_or_byte2;
	    int             max        = fs->max_char_or_byte2;
	    XCharStruct *   per_char   = fs->per_char;
	    int             maxwidth   = fs->max_bounds.width;
	    register char * p;
	    void            dummy_fn();
	    
	    p = GetTextBufferLocation(textBuffer, new.line, NULL);
	    for (i = new.offset, x = startx; i <= maxi && x < maxx; i++) {
		lastx = x;
		x += ((c = p[i]) == '\t') ? 
		    _NextTabFrom(x - startx, tabs, maxwidth) : 
		     _CharWidth(c, fs, per_char, min, max, maxwidth);
	    }
	    if (i != new.offset) {
		/* New Balance-Beam algorithm: if pointer is in right half
		 * of char then place caret AFTER; if pointer is in left half,
		 * then place caret BEFORE it. However, this algorithm is
		 * used only for caret placement and not for ADJUST. 
		 * This is because caret is always *in-between* two chars,
		 * ADJUST extends selection *to* a char.
		 */
		if ((type == OL_CARET) 		&& 
		    (i <= maxi) 		&& 
		    ((x - maxx) < (maxx - lastx)))
		    new.offset = i;
		else
		    new.offset = i - 1;
	    }
	    position = PositionOfLocation(textBuffer, new);
	}
	break;
    case OL_MB_STR_REP:
	{
	    OlTextBufferPtr    textBuffer  = (OlTextBufferPtr)text-> textBuffer;
	    XFontSet	       fs	   = (XFontSet)ctx->primitive.font;
	    register char *    p;
	    wchar_t            wch;
	    register int       clen;
	    int		       tabwidth = XmbTextEscapement(fs, "m", 1);
	    UnitPosition	byte_offset = 
					OlUnitOffsetOfLocation(textBuffer,&new);

	    p = (char *)OlGetTextBufferStringAtLine(textBuffer, new.line, NULL);
	    p = (char *)&p[byte_offset];
	    for (i = new.offset, x = startx,
			clen = mbtowc(&wch, p, MB_CUR_MAX); 
				i <= maxi && x < maxx; i++, p += clen,
					clen = mbtowc(&wch, p, MB_CUR_MAX)) {
		    lastx = x;
		    if (wch == L'\t')
			x += _NextTabFrom(x - startx, tabs, tabwidth);
		    else
		    	x += _CharWidthWC(wch, fs);
	    }

	    if (i != new.offset) {
		/* New Balance-beam algorithm */
	    	if ((type == OL_CARET) 		&&
		    (i <= maxi) 		&& 
		    ((x - maxx) < (maxx - lastx))) 
		    new.offset = i;
		else
		    new.offset = i - 1;
	    }
	    position = OlPositionOfLocation(textBuffer, &new);
	}
	break;
    case OL_WC_STR_REP:
	{
	    OlTextBufferPtr     textBuffer  	= (OlTextBufferPtr)text-> textBuffer;
	    XFontSet	        fs	        = (XFontSet)ctx->primitive.font;
	    register wchar_t *  p;
	    int			tabwidth	= XwcTextEscapement(fs, L"m", 1);
	    
	    p = (wchar_t *)OlGetTextBufferStringAtLine(textBuffer, new.line, NULL);
	    p = &p[new.offset];
	    for (i = new.offset, x = startx; i <= maxi && x < maxx; i++, p++) {

		    lastx = x;
		    if (*p == L'\t')
			x += _NextTabFrom(x - startx, tabs, tabwidth);
		    else
		    	x += _CharWidthWC(*p, fs);
	    }
	    if (i != new.offset) {
		/* New Balance-beam algorithm */
	    	if ((type == OL_CARET)		&&
		    (i <= maxi) 		&& 
		    ((x - maxx) < (maxx - lastx))) 
		    new.offset = i;
 	        else 
		    new.offset = i - 1;
	    }
	    position = OlPositionOfLocation(textBuffer, &new);
	}
	break;
    }
    return (position);
} /* end of PositionFromXY */

/*
 * _RectFromPositions
 *
 * The _RectFromPositions function calculates the bounding box in the
 * TextEdit widget \fIctx\fR for the text between position \fIstart\fR
 * and \fIend\fR.  If the positions are not currently visible the rectangle
 * values (x, y, width, and height) will all be zero (0); otherwise
 * the values are set to define the bounding box for the text.
 *
 * See also:
 *
 * _PositionForXY(3)
 *
 * Synopsis:
 *
 *#include <TextPos.h>
 * ...
 */

extern XRectangle
_RectFromPositions(TextEditWidget ctx, TextPosition start, TextPosition end)
{
    TextEditPart * text       = &ctx-> textedit;
    TextBuffer *   textBuffer = text-> textBuffer;
    WrapTable *    wrapTable  = text-> wrapTable;
    OlFont	   fs         = ctx->primitive.font;
    int            fontht     = text-> lineHeight;
    int            page       = LINES_VISIBLE(ctx);
    XRectangle     rect;
    TextLocation   top;
    TextLocation   bot;
    TextLocation   display;
    TextPosition   position;
    int            topdiff;
    int            botdiff;
    OlStrRep       rep        = text->text_format;

    if (start < 0 || end < 0)
	rect.x = rect.y = rect.width = rect.height = 0;
    else {
	top     = _WrapLocationOfPosition(textBuffer, wrapTable, start);
	bot     = _WrapLocationOfPosition(textBuffer, wrapTable, end);
	display = _WrapLocationOfLocation(wrapTable, text-> displayLocation);

	(void) _MoveToWrapLocation(wrapTable, display, top, &topdiff);
	(void) _MoveToWrapLocation(wrapTable, display, bot, &botdiff);

	if (botdiff < 0 || topdiff > page)
	    rect.x = rect.y = rect.width = rect.height = 0;
	else {
	    topdiff = MAX(0, topdiff);
	    botdiff = MIN(page, botdiff);
	    rect.y = topdiff * fontht + PAGE_T_MARGIN(ctx);
	    rect.height = (botdiff - topdiff + 1) * fontht;
	    if (topdiff == botdiff) {
		char *       p;
		int          offset   = _WrapLineUnitOffset(textBuffer, wrapTable, top);
		TextLocation start_loc;
		TextLocation end_loc;
		UnitPosition su_offs;
		UnitPosition em1u_offs;
		UnitPosition sm1u_offs;

		if (rep == OL_SB_STR_REP) {
		    p         = GetTextBufferLocation(textBuffer, top.line, NULL);
		    start_loc = LocationOfPosition(textBuffer, start);
		    end_loc   = LocationOfPosition(textBuffer, end);
		    su_offs   = start_loc.offset;
		    sm1u_offs = su_offs - 1;
		    em1u_offs = end_loc.offset;
		} else {
		    OlTextBufferPtr mltbp = (OlTextBufferPtr)textBuffer;
		    TextLocation    loc;
		    
		    p = OlGetTextBufferStringAtLine(mltbp, top.line, NULL);
		    OlLocationOfPosition(mltbp, start, &start_loc);
		    OlLocationOfPosition(mltbp, end, &end_loc);
		    loc = start_loc;
		    su_offs   = OlUnitOffsetOfLocation(mltbp, &start_loc);
		    if (loc.offset > 0) {
			loc.offset--;
			sm1u_offs = OlUnitOffsetOfLocation(mltbp, &loc);
		    } else
			sm1u_offs = -1;
		    loc = end_loc;
		    if (loc.offset > 0) {
			loc.offset--;
			em1u_offs = OlUnitOffsetOfLocation(mltbp, &loc);
		    } else
			em1u_offs = -1;
		}

		rect.x = _StringWidth(0, p, offset,
				      sm1u_offs, fs, rep, text->tabs);
		if (end_loc.offset == _WrapLineEnd(textBuffer, wrapTable, bot)) {
		    rect.x    += PAGE_L_MARGIN(ctx) + text-> xOffset;
		    rect.width = PAGEWID(ctx) - rect.x + PAGE_L_MARGIN(ctx);
		} else {
		    rect.width = _StringWidth(rect.x, p, su_offs, em1u_offs,
					      fs, rep, text-> tabs)
			         - rect.x;
		    rect.x    += PAGE_L_MARGIN(ctx) + text-> xOffset;
		}
	    } else {
		rect.x         = PAGE_L_MARGIN(ctx);
		rect.width     = PAGEWID(ctx);
	    }
	}
    }
    return (rect);

} /* end of RectFromPositions */

extern TextPosition
_ScrollDisplayByTextPosDelta(TextEditWidget ctx,
			     int	    text_delta)
{
    TextEditPart * text       = &ctx-> textedit;
    WrapTable *    wrapTable  = text-> wrapTable;
    int            page       = LINES_VISIBLE(ctx);
    TextLocation   current;
    TextLocation   new;
    TextLocation   dummy;
    int            wrap_ln_diff;

    current = _WrapLocationOfLocation(wrapTable, text->displayLocation);
    new = _IncrementWrapLocByTextDelta(ctx, current,
				       &text_delta, &wrap_ln_diff, TRUE);
    dummy.line = -1; dummy.offset = -1;
    (void) _IncrementWrapLocation(wrapTable, new,
				  text->linesVisible, dummy, NULL);
    _MoveDisplay(ctx, text, text->lineHeight, page, new, wrap_ln_diff, False);
    return (text_delta);
}
