#pragma ident	"@(#)TextWrap.c	302.37	97/03/26 lib/libXol SMI"	/* textedit:TextWrap.c 1.6	*/

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


#include <wctype.h>
#include <widec.h>
#include <libintl.h>

#include <values.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/Dynamic.h>
#include <Xol/Oltextbuff.h>
#include <Xol/OlStrMthdsI.h>
#include <Xol/OpenLookP.h>
#include <Xol/Scrollbar.h>
#include <Xol/TextEdit.h>
#include <Xol/TextEditP.h>
#include <Xol/TextWrap.h>
#include <Xol/TextUtil.h>
#include <Xol/Util.h>
#include <Xol/buffutil.h>
#include <Xol/textbuff.h>


static int		is_non_printable_char(int rc);
static int 		_GetmbTabWidth( XFontSet );
static int 		_GetwcTabWidth( XFontSet );
static _OlNonPrintableCharFunc IsNonPrintableChar = is_non_printable_char;
void _OLSetScrolledWindow_HScrollbars();	/* New for 1222062 */


/*
 * _FirstWrapLine
 *
 * The \fI_FirstWrapLine\fR function returns the first valid wrap location
 * for the given \fIwrapTable\fR.
 *
 * See also:
 *
 * _LastWrapLine(3), _LastDisplayedWrapLine(3)
 *
 * Synopsis:
 *#include <TextWrap.h>
 * ...
 */
extern TextLocation
_FirstWrapLine(WrapTable * wrapTable)
{
    static TextLocation location;
				/* initialized by compiler to zero */
    location.line = wrapTable->cur_start;
    return (location);
}/* end of _FirstWrapLine */

/*
 * _LastDisplayedWrapLine
 *
 * The \fI_LastDisplayedWrapLine\fR function returns the wrap location
 * corresponding to the first displayed line on the last pane of display
 * given the pane size \fIpage\fR for \fIwrapTable\fR.  
 *
 * See also:
 *
 * _FirstWrapLine(3), _LastWrapLine(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
extern TextLocation
_LastDisplayedWrapLine(WrapTable * wrapTable,
		       int         page)
{
    return(_IncrementWrapLocation(wrapTable, _LastWrapLine(wrapTable), 
				  -(page - 1), _FirstWrapLine(wrapTable), NULL));
} /* end of _LastDisplayedWrapLine */

/*
 * _LastWrapLine
 *
 *
 * The \fI_LastWrapLine\fR function returns the last valid wrap location
 * in \fIwrapTable\fR
 *
 * See also:
 * 
 * _FirstWrapLine(3), _LastDisplayedWrapLine(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
extern TextLocation
_LastWrapLine(WrapTable * wrapTable)
{
    static TextLocation location;
    WrapContents *      wrc = wrapTable->contents;

    location.line   = wrapTable->cur_end;
    location.offset = wrc->p[location.line]->used - 1;
    return (location);
} /* end of _LastWrapLine */

/*
 * _LineNumberOfWrapLocation
 *
 * The \fI_LineNumberOfWrapLocation\fR function is used to calculate
 * the number of display lines between the first wrap location and
 * the wrap \fIlocation\fR in \fIwrapTable\fR.
 *
 * See also:
 *
 * _NumberOfWrapLines(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
extern int
_LineNumberOfWrapLocation(WrapTable *  wrapTable,
			  TextLocation location)
{
    register int	i;
    register int	cnt = location.offset;
    WrapContents *      wrc = wrapTable->contents;

    for (i = 0; i <= wrapTable->cur_end && i < location.line; i++)
	cnt += wrc->p[i]->used;
    return (cnt);
} /* end of _LineNumberOfWrapLocation */

/*
 * _NumberOfWrapLines
 *
 * The \fI_NumberOfWrapLines\fR function is used to calculate the number
 * of display lines in \fIwrapTable\fR.
 *
 * See also:
 *
 * _LineNumberOfWrapLocation(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
extern int
_NumberOfWrapLines(WrapTable * wrapTable)
{
    register int i;
    register int cnt = 0;
    WrapContents *      wrc = wrapTable->contents;

    for (i = wrapTable->cur_start; i <= wrapTable->cur_end; i++)
	cnt += wrc->p[i]->used;
    return (cnt);
} /* end of _NumberOfWrapLines */

/*
 * _UpdateWrapTable
 *
 * The \fI_UpdateWrapTable\fR procedure is used to update the wrap table
 * associated with the TextEdit widget \fIctx\fR for the real lines
 * between \fIstart\fR and \fIend\fR inclusive.
 *
 * See also:
 *
 * _BuildWrapTable(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
typedef TextPosition (*WrapLineFunc)(int,
				     int,
				     OlStr,
				     TextPosition,
				     UnitPosition *,
				     OlFont,
				     TabTable,
				     OlWrapMode);

static WrapLineFunc wl_funcs[NUM_SUPPORTED_REPS] = {
    _WrapLine,
    _WrapLineMB,
    _WrapLineWC
    };
				     
extern void
_UpdateWrapTable(TextEditWidget ctx,
		 TextLine       start,
		 TextLine       end)
{
    TextEditPart * text            = &ctx->textedit;
    WrapTable *    wrapTable       = text->wrapTable;
    WrapContents * wrc             = wrapTable->contents;
    TextBuffer *   textBuffer      = text->textBuffer;
    OlFont         fs              = ctx->primitive.font;
    TabTable       tabs            = text->tabs;
    OlWrapMode     wrapMode        = text->wrapMode;
    TextLine       i;
    TextPosition   j;
    int            x               = HGAP(ctx) + PAGE_L_MARGIN(ctx);
    int            maxx;
    int		   old_maxx	   = text->maxX;
    OlStr          p;
    TextPosition   pos;
    TextPosition   nextpos;
    OlStrRep       rep             = text->text_format;
    WrapLineFunc   wrap_line_fn    = wl_funcs[rep];
    int		   wrap_lns_added  = 0;

    if (wrapMode == OL_WRAP_OFF) {
	int l = (rep == OL_SB_STR_REP)?
	          LinesInTextBuffer(textBuffer):
		  OlLinesInTextBuffer((OlTextBufferPtr)textBuffer);

	maxx = (l == 1)? 0 : text->maxX;
    } else
	maxx = PAGE_R_MARGIN(ctx);
    
    for (i = start; i <= end; i++) {
	if(wrc->p[i] == NULL) 
		wrc->p[i] = (WrapLocation *)
				AllocateBuffer(sizeof(wrc->p[i]->p[0]), 1);
	p         = (rep == OL_SB_STR_REP)?
	              (OlStr)GetTextBufferLocation(textBuffer, i, (TextLocation *)NULL):
	              OlGetTextBufferStringAtLine((OlTextBufferPtr)textBuffer,
						  i, (TextLocation *)NULL);
	if (wrapMode == OL_WRAP_OFF) {
	    if (0 == wrc->p[i]->size)
		GrowBuffer((Buffer *)wrc-> p[i], 1);
	    wrc->p[i]->p[0] = 0;
	    wrc->p[i]->used = 1;
	    x               = _StringWidth(0, p, 0,
					 str_methods[rep].StrNumUnits(p),
							   fs, rep, tabs);
	    maxx            = MAX(x, maxx);
	    wrap_lns_added++;
	} else {
	    UnitPosition	uoffs = 0;
	   
	    for (j = 0, pos = 0; pos != EOF; pos = nextpos, j++) {
		if (j == wrc->p[i]->size)
		    GrowBuffer((Buffer *)wrc->p[i], 1);
		nextpos         = wrap_line_fn(x, maxx, p, pos, &uoffs, fs, tabs, wrapMode);
		wrc->p[i]->p[j] = pos;
		if (nextpos == pos)
		    nextpos++;
		wrap_lns_added++;
	    }
	    wrc->p[i]->used = j;
	}
    }
	/*
	 *  Update the Horizontal scrollbars, if the value of maxX is changed.
	 *  ESC#502991,BID#1222062
	 */ 
	if(text->maxX != maxx){
	   text->maxX = maxx;
	   _OLSetScrolledWindow_HScrollbars(ctx, ctx->core.width, ctx->core.height);
	}

    text->lineCount += wrap_lns_added; /* _NumberOfWrapLines(wrapTable); */
} /* end of _UpdateWrapTable */

/*
 * _BuildWrapTable
 *
 * The \fI_BuildWrapTable\fR procedure is used to totally construct the
 * wrap table for the TextEdit widget \fIctx\fR.
 *
 * See also:
 *
 * _UpdateWrapTable(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */

extern Boolean
_UpdateWrapTableAbove(TextEditWidget 	ctx,
		      Boolean		do_double)
{
    TextEditPart * text       = &ctx-> textedit;
    WrapTable *    wrapTable  = text-> wrapTable;
    WrapContents * wrc	      = wrapTable->contents;
    int		   prev_start = wrapTable->cur_start;
    Boolean	   ret_val    = True;

    if (prev_start > 0) {
	int	   start;
	int	   end;

	if (do_double)
	    start = MAX(prev_start - text->linesVisible*2, 0);
	else
	    start = MAX(prev_start - text->linesVisible, 0);
	end   = prev_start - 1;
	wrapTable->cur_start  = start;
	_UpdateWrapTable(ctx, start, end);
	ret_val = False;
    }

    return(ret_val);
}

extern Boolean
_UpdateWrapTableBelow(TextEditWidget	ctx,
		      Boolean		do_double)
{
    TextEditPart * text       = &ctx-> textedit;
    WrapTable *    wrapTable  = text-> wrapTable;
    WrapContents * wrc	      = wrapTable->contents;
    int            num_lines  = wrc->used;
    int		   prev_end   = wrapTable->cur_end;
    Boolean	   ret_val    = True;

    if (prev_end+1 < num_lines) {
	int 	   start;
	int	   end;

	start = prev_end+1;
	if (do_double)
	    end   = MIN(prev_end + text->linesVisible*2, num_lines - 1);
	else
	    end   = MIN(prev_end + text->linesVisible, num_lines - 1);
	wrapTable->cur_end   = end;
	_UpdateWrapTable(ctx, start, end);
	ret_val = False;
    }
    return(ret_val);
}

static Boolean
_BackgroundUpdateWrapTable(XtPointer client_data)
{
    TextEditWidget ctx = (TextEditWidget) client_data;
    TextEditPart * text       = &ctx-> textedit;
    WrapTable *    wrapTable  = text-> wrapTable;
    WrapContents * wrc	      = wrapTable->contents;
    int            num_lines  = wrc->used;
    int		   prev_end   = wrapTable->cur_end;
    Boolean	   ret_val;

    ret_val  = _UpdateWrapTableAbove(ctx, !(prev_end+1 < num_lines));
    ret_val &= _UpdateWrapTableBelow(ctx, ret_val);
    if (ret_val == True)
	text->wrap_work_proc_id = (XtWorkProcId) NULL;
    return(ret_val);
}

extern void
_BuildWrapTable(TextEditWidget ctx)
{
    TextEditPart * text       = &ctx-> textedit;
    WrapTable *    wrapTable  = text-> wrapTable;
    TextBuffer *   textBuffer = text-> textBuffer;
    WrapContents * wrc;
    TextLine       i;
    int            num_lines;
    int            space_needed;
    OlStrRep       rep        = text->text_format;

    text->linesVisible = FONTHT(ctx) ? PAGE_LINE_HT(ctx) : 0;
    text->charsVisible = text->charWidth ? PAGEWID(ctx) / text->charWidth : 0;

    if(wrapTable != NULL) {
	if(text->linesVisible == 0 	||
	   text->charsVisible == 0	)	 
		return;
    } else {
		if(text->linesVisible == 0)
			text->linesVisible = 1;
		if(text->charsVisible == 0)
			text->charsVisible = 1;
    } 

    if (wrapTable != (WrapTable *)NULL)
	if (!wrapTable->build_wrap || text->prev_width == ctx->core.width)
	    return;
    num_lines = (rep == OL_SB_STR_REP) ?
	          LinesInTextBuffer(textBuffer):
		  OlLinesInTextBuffer((OlTextBufferPtr)textBuffer);
    if (wrapTable != (WrapTable *)NULL) {
	wrc = wrapTable->contents;
	for (i = wrapTable->cur_start; i <= wrapTable->cur_end; i++) {
	    FreeBuffer((Buffer *)wrc-> p[i]);
	    wrc->p[i] = NULL;
	}

	space_needed = num_lines + 1 - wrc->size;
	if (space_needed != 0) {
	    GrowBuffer((Buffer *)wrc, space_needed);
	    for(i=wrc->size - space_needed; i < wrc->size; i++)
		wrc->p[i] = NULL;
	}
    } else {
	wrapTable = text->wrapTable = (WrapTable *)XtMalloc(sizeof(WrapTable));
	wrapTable->ctx = ctx;
	wrapTable->build_wrap = TRUE;
	wrapTable->text_format = rep;
	wrc = text->wrapTable->contents = 
	    (WrapContents *)AllocateBuffer(sizeof(wrc->p[0]), num_lines + 1);
	for(i=0; i < wrc->size; i++)
		wrc->p[i] = NULL;
    }
    i = wrc->used = num_lines;
    text->maxX = 0;
    {
	int visline	= text->displayLocation.line;
	int start;
	int end;
	
	start = MAX(-text->linesVisible + visline, 0);
	end   = MIN(visline + text->linesVisible - 1, num_lines - 1);

	wrapTable->cur_start = start;
	wrapTable->cur_end   = end;
	text->lineCount      = 0;
	_UpdateWrapTable(ctx, start, end);
	if (text->wrap_work_proc_id != (XtWorkProcId) NULL)
	    XtRemoveWorkProc(text->wrap_work_proc_id);
	text->wrap_work_proc_id = XtAppAddWorkProc(
			 XtWidgetToApplicationContext((Widget) ctx),
			 _BackgroundUpdateWrapTable, (XtPointer) ctx);
    }
    wrc->p[i] = (WrapLocation *)AllocateBuffer(sizeof(wrc->p[i]->p[0]), 1);
    wrc->p[i]->used = 0;
    text->prev_width = ctx->core.width;
} /* end of _BuildWrapTable */

/*
 * _GetNextWrappedLine
 *
 * The \fI_GetNextWrappedLine\fR function is used to retrieve the string
 * stored in the TextBuffer \fItextBuffer\fR at the \fIlocation\fR of
 * the \fIwrapTable\fR.  The function returns the pointer to the string
 * and increments the wrap location for use in subsequent calls to
 * retrieve the next wrap line.
 *
 * Synopsis:
 * 
 *#include <TextWrap.h>
 * ...
 */

OlStr
_GetNextWrappedLine(TextBuffer* textBuffer, WrapTable* wrapTable,
	TextLocation* location)
{
	OlStrRep		rep = wrapTable->text_format;
	WrapContents*		wrc = wrapTable->contents;
	OlStr			p = NULL;
	char*			cp;

	switch (rep) {

	case OL_SB_STR_REP:
		cp = GetTextBufferLocation(textBuffer, location->line,
			NULL);
		if (cp != (char*)NULL) {
			cp = &cp[wrc->p[location->line]->p[location->offset]];
			if (++location->offset == wrc->p[location->line]->used) {
				location->line++;
				location->offset = 0;
			}
		}
		p = (OlStr)cp;
		break;

	case OL_MB_STR_REP:
		{
			TextLocation		loc;
			OlTextBufferPtr		mltx =
			(OlTextBufferPtr) textBuffer;

			cp = (char*)OlGetTextBufferStringAtLine(mltx,
				location->line, &loc);

			if (cp != (char*)NULL) {
				loc.offset =
				wrc->p[location->line]->p[location->offset];

				cp = &cp[OlUnitOffsetOfLocation(mltx, &loc)];
				if (++location->offset ==
					wrc->p[location->line]->used) {

					location->line++;
					location->offset = 0;
				}
			}
		}
		p = (OlStr)cp;
		break;

	case OL_WC_STR_REP:
		{
			TextLocation		loc;
			OlTextBufferPtr		mltx =
				(OlTextBufferPtr) textBuffer;
			wchar_t*		wp;

			wp = (wchar_t*)OlGetTextBufferStringAtLine(mltx,
				location->line, &loc);

			if (wp != (wchar_t*)NULL) {
				loc.offset = 
				wrc->p[location->line]->p[location->offset];

				p = (OlStr)&wp[OlUnitOffsetOfLocation(mltx,
					&loc)];

				if (++location->offset ==
					wrc->p[location->line]->used) {

					location->line++;
					location->offset = 0;
				}
			}
		}
		break;
	}

	return (p);
}


/*
 * _IncrementWrapLocation
 *
 * The \fI_IncrementWrapLocation\fR function is used to calculate the
 * wrap location in the \fIwrapTable\fR which is offset \fIn\fR lines
 * from the \fIcurrent\fR wrap location.  This calculation is governed
 * by the \fIlimit\fR location.  The routine reports the actual number
 * of wrap lines between \fIcurrent\fR and the return location in
 * the \fIreallines\fR return value.
 *
 * See also:
 *
 * _MoveToWrapLocation(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */

extern TextLocation
_IncrementWrapLocation(WrapTable *  wrapTable,
		       TextLocation current,
		       int          n,
		       TextLocation limit,
		       int *        reallines)
{
    register int i;
    Boolean      more_lines   = TRUE;
    Boolean	 wrap_updated = FALSE;

    if (n < 0) {
	TextLocation    top_line = _FirstWrapLine(wrapTable);

	if (current.line < top_line.line)
	    if (top_line.line > 0) {
		_UpdateWrapTableAbove(wrapTable->ctx, True);
		top_line = _FirstWrapLine(wrapTable);
		wrap_updated = TRUE;
	    } else
		more_lines = FALSE;

	if (more_lines)
	    for (i = 0; !SameTextLocation(current, limit) && i > n; i--) {
		if (--current.offset < 0) {
		    if (--current.line < top_line.line)
			if (top_line.line > 0) {
			    _UpdateWrapTableAbove(wrapTable->ctx, True);
			    top_line = _FirstWrapLine(wrapTable);
			    wrap_updated = TRUE;
			} else {
			    current.line++;
			    current.offset++;
			    break;
			}
		    current.offset = wrapTable->contents->p[current.line]
			                                       ->used-1;
		}
	    }
    } else if (n > 0) {
	TextLocation    bot_line = _LastWrapLine(wrapTable);

	if (current.line > bot_line.line)
	    if (bot_line.line+1 < wrapTable->contents->used) {
		_UpdateWrapTableBelow(wrapTable->ctx, True);
		bot_line = _LastWrapLine(wrapTable);
		wrap_updated = TRUE;
	    } else
		more_lines = FALSE;

	if (more_lines)
	    for (i = 0; !SameTextLocation(current, limit) && i < n; i++) {
		if (++current.offset >
		    wrapTable->contents->p[current.line]->used - 1)
		{
		    if (++current.line > bot_line.line)
			if (bot_line.line+1 < wrapTable->contents->used) {
			    _UpdateWrapTableBelow(wrapTable->ctx, True);
			    bot_line = _LastWrapLine(wrapTable);
			    wrap_updated = TRUE;
			} else {
			    current.line--;
			    current.offset--;
			    break;
			}
		    current.offset = 0;
		}
	    }
    } else
	i = 0;
    if (reallines != NULL)
	*reallines = i;
    if (wrap_updated)
	(void) _BGDisplayChange(wrapTable->ctx);
    return (current);
} /* end of _IncrementWrapLocation */

/*
 * _MoveToWrapLocation
 *
 * The \fI_MoveToWrapLocation\fR function is used to calculate the number
 * of display lines between \fIcurrent\fR and \fIlimit\fR in the
 * \fIwrapTable\fR.  This value is returned in the \fIreallines\fR
 * return parameter.
 *
 * See also:
 *
 * _IncrementWrapLocation(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
extern TextLocation
_MoveToWrapLocation(WrapTable *  wrapTable,
		    TextLocation current,
		    TextLocation limit,
		    int *        reallines)
{
    register int	i;
    Boolean		more_lines = TRUE;
    Boolean		wrap_updated = FALSE;

    if (current.line == limit.line)
	if (current.offset == limit.offset)
	    i = 0;
	else
	    i = limit.offset - current.offset;
    else if (current.line < limit.line) {
	TextLocation    bot_line = _LastWrapLine(wrapTable);

	if (current.line > bot_line.line)
	    if (bot_line.line+1 < wrapTable->contents->used) {
		_UpdateWrapTableBelow(wrapTable->ctx, True);
		bot_line = _LastWrapLine(wrapTable);
		wrap_updated = TRUE;
	    } else
		more_lines = FALSE;
	if (more_lines)
	    for (i = 0; !SameTextLocation(current, limit); i++) {
		if (++current.offset >
		    wrapTable->contents->p[current.line]->used - 1)
		{
		    if (++current.line > bot_line.line)
			if (bot_line.line+1 < wrapTable->contents->used) {
			    _UpdateWrapTableBelow(wrapTable->ctx, True);
			    bot_line = _LastWrapLine(wrapTable);
			    wrap_updated = TRUE;
			} else {
			    current.line--;
			    current.offset--;
			    break;
			}
		    current.offset = 0;
		}
	    }
    } else {
	TextLocation    top_line = _FirstWrapLine(wrapTable);
	
	if (current.line < top_line.line)
	    if (top_line.line > 0) {
		_UpdateWrapTableAbove(wrapTable->ctx, True);
		top_line = _FirstWrapLine(wrapTable);
		wrap_updated = TRUE;
	    } else
		more_lines = FALSE;
	if (more_lines)
	    for (i = 0; !SameTextLocation(current, limit); i--) {
		if (--current.offset < 0) {
		    if (--current.line < top_line.line)
			if (top_line.line > 0) {
			    _UpdateWrapTableAbove(wrapTable->ctx, True);
			    top_line = _FirstWrapLine(wrapTable);
			    wrap_updated = TRUE;
			} else {
			    current.line++;
			    current.offset++;
			    break;
			}
		    current.offset =
			wrapTable->contents->p[current.line]->used - 1;
		}
	    }
    }
    if (reallines != NULL)
	*reallines = i;
    if (wrap_updated)
	(void) _BGDisplayChange(wrapTable->ctx);
    return (current);
} /* end of _MoveToWrapLocation */

/*
 * _PositionOfWrapLocation
 *
 * The \fI_PositionOfWrapLocation\fR function is used to convert a
 * \fIlocation\fR in \fIwrapTable\fR to a TextPosition in \fItextBuffer\fR.
 *
 * See also:
 *
 * _LocationOfWrapLocation(3), _WrapLocationOfPosition(3), et al
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
extern TextPosition
_PositionOfWrapLocation(TextBuffer * textBuffer,
			WrapTable *  wrapTable,
			TextLocation location)
{
    location = _LocationOfWrapLocation(wrapTable, location);
    if (wrapTable->text_format == OL_SB_STR_REP)
	return(PositionOfLocation(textBuffer, location));
    else
	return(OlPositionOfLocation((OlTextBufferPtr)textBuffer, &location));
} /* end of _PositionOfWrapLocation */

/*
 * _LocationOfWrapLocation
 *
 * The \fI_LocationOfWrapLocation\fR function is used to convert a
 * \fIlocation\fR in \fIwrapTable\fR to a Location.
 *
 * See also:
 *
 * _PositionOfWrapLocation(3), et al
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */

extern TextLocation
_LocationOfWrapLocation(WrapTable *  wrapTable,
			TextLocation location)
{
    location.offset = wrapTable->contents->p[location.line]->p[location.offset];
    return(location);
} /* end of _LocationOfWrapLocation */

/*
 * _WrapLocationOfPosition
 *
 * The \fI_WrapLocationOfPosition\fR function is used to convert a
 * \fIposition\fR in \fItextBuffer\fR to a wrap location in \fIwrapTable\fR.
 *
 * See also:
 *
 * _LocationOfWrapLocation(3), _PositionOfWrapLocation(3), et al
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
extern TextLocation
_WrapLocationOfPosition(TextBuffer * textBuffer,
			WrapTable *  wrapTable,
			TextPosition position)
{
    TextLocation l;

    if (wrapTable->text_format == OL_SB_STR_REP)
	l = LocationOfPosition(textBuffer, position);
    else
	OlLocationOfPosition((OlTextBufferPtr)textBuffer, position, &l);
    return(_WrapLocationOfLocation(wrapTable, l));
} /* end of _WrapLocationOfPosition */

/*
 * _WrapLocationOfLocation
 *
 * The \fI_WrapLocationOfLocation\fR function is used to convert a
 * \fIlocation\fR to a wrap location in \fIwrapTable\fR.
 *
 * See also:
 *
 * _LocationOfWrapLocation(3), _PositionOfWrapLocation(3), et al
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
extern TextLocation
_WrapLocationOfLocation(WrapTable *  wrapTable,
			TextLocation location)
{
    register int i;
    TextLine start, end;

    start = end = 0;

    if(wrapTable->contents->p[location.line] == NULL) 	{ 

	if(location.line > wrapTable->cur_end) {
		start = wrapTable->cur_end + 1;
		end = location.line;
	 } else if(location.line < wrapTable->cur_start) {
		start = location.line;
		end = wrapTable->cur_start - 1;
	}

	if(start != wrapTable->cur_start 	||
		end !=	wrapTable->cur_end	)
		_UpdateWrapTable(wrapTable->ctx, start, end);
	if(location.line > wrapTable->cur_end) 
		wrapTable->cur_end = location.line;
	else if(location.line < wrapTable->cur_start)
		wrapTable->cur_start = location.line;
    }

    for (i = 0; i < wrapTable->contents->p[location.line]->used; i++)
	if (wrapTable->contents->p[location.line]->p[i] > location.offset)
	    break;
    location.offset = i - 1;
    return (location);
} /* end of _WrapLocationOfLocation */


/*
 * _WrapLineOffset
 *
 * The \fI_WrapLineOffset\fR function decodes the TextPosition of a 
 * line in \fIwrapTable\fR at \fIlocation\fR.
 *
 * See also:
 *
 * _WrapLineLength(3), _WrapLineEnd(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 *
 */
extern TextPosition
_WrapLineOffset(WrapTable *  wrapTable,
		TextLocation location)
{
    return (wrapTable->contents->p[location.line]->p[location.offset]);
} /* end of _WrapLineOffset */


extern UnitPosition
_WrapLineUnitOffset(TextBuffer* textbuf, WrapTable*  wrapTable, 
	TextLocation location)
{
	TextLocation		loc = location;

	loc.offset = wrapTable->contents->p[loc.line]->p[loc.offset];

	if (wrapTable->text_format == OL_SB_STR_REP)
		return (loc.offset);
	else
		return (OlUnitOffsetOfLocation((OlTextBufferPtr) textbuf, &loc));
}


/*
 * _WrapLineLength
 *
 * The \fI_WrapLineLength\fR function calculates the length of a 
 * \fItextBuffer\fR line in \fIwrapTable\fR at \fIlocation\fR in chars.
 *
 * See also:
 *
 * _WrapLineOffset(3), _WrapLineEnd(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
extern TextPosition
_WrapLineLength(TextBuffer * textBuffer,
		WrapTable *  wrapTable,
		TextLocation location)
{
    return (1 + 
	    _WrapLineEnd(textBuffer, wrapTable, location) - 
	    _WrapLineOffset(wrapTable, location));
} /* end of _WrapLineLength */

/*
 * _WrapLineNumUnits
 *
 * The \fI_WrapLineLength\fR function calculates the length of a 
 * \fItextBuffer\fR line in \fIwrapTable\fR at \fIlocation\fR in number of storage units.
 *
 * See also:
 *
 * _WrapLineOffset(3), _WrapLineEnd(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
extern UnitPosition
_WrapLineNumUnits(TextBuffer * textBuffer,
		  WrapTable *  wrapTable,
		  TextLocation location) /* a wrap location */
{
    TextPosition   right;
    UnitPosition   unitlen;
    int            line = location.line;
    int            offs = location.offset;
    WrapContents * wrc  = wrapTable->contents;

    if (wrapTable->text_format == OL_SB_STR_REP) {
	right = ((wrc->p[line]->used - 1) == offs)?
	         1 + LastCharacterInTextBufferLine(textBuffer, line):
		 wrc->p[line]->p[offs+1];
	unitlen = right - wrc->p[line]->p[offs];
    } else {
	OlTextBufferPtr mltx = (OlTextBufferPtr)textBuffer;
	
	if(wrc->p[line]->used - 1 == offs) 
		right = OlNumUnitsInTextBufferLine(mltx, line); 
	else {
	        location.offset = wrc->p[line]->p[offs+1];
		right = OlUnitOffsetOfLocation(mltx, &location);
	}
	location.offset = wrc->p[line]->p[offs];
	unitlen           = right - OlUnitOffsetOfLocation(mltx, &location);
    }
    return (unitlen);
} /* end of _WrapLineNumUnits */

/*
 * _WrapLineEnd
 *
 * The \fI_WrapLineEnd\fR function calculates the last TextPosition in a 
 * \fItextBuffer\fRline in \fIwrapTable\fR at \fIlocation\fR.
 *
 * See also:
 *
 * _WrapLineOffset(3), _WrapLineLength(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
extern TextPosition
_WrapLineEnd(TextBuffer * textBuffer,
	     WrapTable *  wrapTable,
	     TextLocation location) /* this is a wrap location */
{
    TextPosition   right;
    WrapContents * wrc  = wrapTable->contents;
    int            line = location.line;
    int            offs = location.offset;

    if (wrapTable->text_format == OL_SB_STR_REP) {
	right = ((wrc->p[line]->used - 1) == offs)?
	         LastCharacterInTextBufferLine(textBuffer, line):
		 wrc->p[line]->p[offs+1] - 1;
    } else {
	right = ((wrc->p[line]->used - 1) == offs)?
	         OlLastCharInTextBufferLine((OlTextBufferPtr)textBuffer, line):
		 wrc->p[line]->p[offs+1] - 1;
    }
    return(right);
} /* end of _WrapLineEnd */

/*
 * _StringOffsetAndPosition
 *
 * The \fI_StringOffsetAndPosition\fR procedure is used to calculate the
 * character offset and x location ...
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
extern void
_StringOffsetAndPosition(OlStr          s,
			 UnitPosition   start,
			 UnitPosition   end,
			 OlFont         fontsp,
			 OlStrRep       rep,
			 TabTable       tabs,
			 int            minx,
			 int *          retx,
			 UnitPosition * reti)
{
    int            x;

    switch (rep) {
    case OL_SB_STR_REP:
	{
	    XFontStruct *   fs         = (XFontStruct *)fontsp;
	    char        *   ss         = (char *)s;
	    int             min        = fs->min_char_or_byte2;
	    int             max        = fs->max_char_or_byte2;
	    XCharStruct *   per_char   = fs->per_char;
	    int             maxwidth   = fs->max_bounds.width;
	    register char * p;
	    char *          q;
	    int             c;
	    
	    for (x = 0, p = &ss[start], q = &ss[end]; x < minx && p < q; p++)
		x += ((c = *p) == '\t') ? 
		        _NextTabFrom(x, tabs, maxwidth) : 
		        _CharWidth(c, fs, per_char, min, max, maxwidth);
	    *reti = start + p - ss;
	}
	break;
    case OL_MB_STR_REP:
	{
	    XFontSet	       fs	   = (XFontSet)fontsp;
	    char *             ms          = (char *)s;
	    register char *    cstart;
	    register char *    p;
	    char          *    q;
	    wchar_t            wch;
	    register int       clen;
	    int		       tabwidth = _GetmbTabWidth(fs);

	    for (x = 0, cstart = p = &ms[start], q = &ms[end]; 
		      (clen = mbtowc(&wch, p, MB_CUR_MAX)) > 0 &&
					 x < minx && p < q; p+=clen) {
		x += (wch == L'\t' ? _NextTabFrom(x, tabs, tabwidth) :
				_CharWidthWC(wch, fs));
				 
	    } /* end of for */

	    *reti = start + p - ms;
	}
	break;
    case OL_WC_STR_REP:
	{
	    XFontSet	        fs	= (XFontSet)fontsp;
	    wchar_t *           ws      = (wchar_t *)s;
	    register wchar_t *  cstart;
	    register wchar_t *  p;
	    wchar_t *           q;
	    int			tabwidth = _GetwcTabWidth(fs);

	    for (x = 0, cstart = p = &ws[start], q = &ws[end]; 
				*p != L'\0' &&	x < minx && p < q; p++) {
	     x += (*p == L'\t' ? _NextTabFrom(x, tabs, tabwidth) :
						_CharWidthWC(*p, fs));
	    } /* end of for */

	    *reti = start + p - ws;
	}
	break;
    }
    *retx = x - minx;
} /* end of _StringOffsetAndPosition */

/*
 * _StringWidth
 *
 * The \fI_StringWidth\fR function calculates the length in pixels
 * of the string \fIs\fR between \fIstart\fR end \fIend\fR using the
 * XFontstruct pointer \fIfs\fR and TabTable \fItabs\fR.
 *
 * See also:
 * 
 * _CharWidth(3), _NextTabFrom(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
extern int
_StringWidth(int           x,
	     OlStr         s,
	     UnitPosition  start,
	     UnitPosition  end,
	     OlFont        fontsp,
	     OlStrRep      rep,
	     TabTable      tabs)
{
    switch (rep) {
    case OL_SB_STR_REP:
	{
	    XFontStruct *   fs         = (XFontStruct *)fontsp;
	    char *          ss         = (char *)s;
	    int             min        = fs->min_char_or_byte2;
	    int             max        = fs->max_char_or_byte2;
	    XCharStruct *   per_char   = fs->per_char;
	    int             maxwidth   = fs->max_bounds.width;
	    register char * p;
	    char *          q;
	    int             c;
	    
	    for (p = &ss[start], q = &ss[end]; p <= q; p++)
		x += ((c = *p) == '\t') ? 
		        _NextTabFrom(x, tabs, maxwidth) : 
		        _CharWidth(c, fs, per_char, min, max, maxwidth);
	}
	break;
    case OL_MB_STR_REP:
	{
	    XFontSet	       fs	   = (XFontSet)fontsp;
	    char *             ms         = (char *)s;
	    register char *    cstart;
	    register char *    p = &ms[start];
	    char          *    q = &ms[end];
	    wchar_t            wch;
	    register int       clen;
            int                tabwidth = _GetmbTabWidth(fs); 

	    for (cstart = p; p <= q && (clen = 
				mbtowc(&wch,p,MB_CUR_MAX)) > 0; p+=clen)
	    {
		if (wch == L'\t') {
		    x += XmbTextEscapement(fs, cstart, p - cstart);
		    cstart = p + 1;
		    x += _NextTabFrom(x, tabs, tabwidth);
		}
	    }
	    if (cstart < p)
		x += XmbTextEscapement(fs, cstart, p - cstart);
	}
	break;
    case OL_WC_STR_REP:
	{
	    XFontSet	        fs	= (XFontSet)fontsp;
	    wchar_t *           ws      = (wchar_t *)s;
	    register wchar_t *  cstart;
	    register wchar_t *  p;
	    wchar_t *           q;
            int                 tabwidth = _GetwcTabWidth(fs); 

	    for (cstart = p = &ws[start], q = &ws[end]; 
				p <= q && *p != L'\0'; p++) {
		if (*p == L'\t') {
		    x += XwcTextEscapement(fs, cstart, p - cstart);
		    cstart = p + 1;
		    x += _NextTabFrom(x, tabs, tabwidth);
		}
	    }
	    if (cstart < p)
		x += XwcTextEscapement(fs, cstart, p - cstart);
	}
	break;
    }
    return(x);
} /* end of _StringWidth */

/*
 * _CharWidth
 *
 * The \fI_CharWidth\fR function is used to calculate the width of the
 * character \fIc\fR using the XFontStruct information pointed to by \fIfs\fR,
 * \fIper_char\fR, \fImin\fR, \fImax\fR, and \fImaxwidth\fR metrics.
 *
 * See also:
 *
 * _StringWidth(3), _NextTabFrom(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
extern int
_CharWidth(int           ic,
	   XFontStruct * fs,
	   XCharStruct * per_char,
	   int           min,
	   int           max,
	   int           maxwidth)
{
    int           nonPrintable;
    int           width;
    unsigned char c = (unsigned char)ic;

#define NULL_IS_ZERO_WIDTH
    if (c == '\0') 
#ifdef NULL_IS_ZERO_WIDTH
	return (0);
#else
    c = ' ';
#endif

    nonPrintable = (*IsNonPrintableChar)(c);   
    if (nonPrintable)
	c += '@';
    if (per_char && min <= (unsigned int)c && (unsigned int)c <= max)
	width = per_char[c - min].width;
    else
	width = maxwidth;
    if (nonPrintable) {
	unsigned char hat;

	hat = '^';
	if (per_char && min <= (unsigned int)hat && (unsigned int)hat <= max)
	    width += per_char[hat - min].width;
	else
	    width += maxwidth;
    }
    return (width);
} /* end of _CharWidth */

/*
 * _CharWidthMB
 *
 * The \fI_CharWidthMB\fR function is used to calculate the width of the
 * mb character \fIc\fR using the XFontSet information pointed to by \fIfs\fR.
 *
 * See also:
 *
 * _CharWidth(3), _CharWidthWC(3), _StringWidth(3), _NextTabFrom(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
extern int
_CharWidthMB(char *   ch,
	     XFontSet fs)
{
    wchar_t  wch;

    mbtowc(&wch, ch, MB_CUR_MAX);
    return _CharWidthWC(wch, fs);
}

/*
 * _CharWidthWC
 *
 * The \fI_CharWidthWC\fR function is used to calculate the width of the
 * wide character \fIc\fR using the XFontSet information pointed to by \fIfs\fR.
 *
 * See also:
 *
 * _CharWidth(3), _CharWidthMB(3), _StringWidth(3), _NextTabFrom(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
extern int
_CharWidthWC(wchar_t       ch,
	     XFontSet fs)
{
    if (ch == L'\0') 
	return (0);
    else {
	int           num_c;
	XRectangle    dum;
	XRectangle    log_ext;
	wchar_t       wc[2];

	wc[0] = ch;
	wc[1] = L'\0';

	XwcTextPerCharExtents(fs, wc, 1, &dum, &log_ext, 1, &num_c, &dum, &dum);
	if (num_c == 1)
	    return(log_ext.width);
	else
	    return(0);
    }
} /* end of _CharWidthWC */

/*
 * _NextTabFrom
 *
 * The \fI_NextTabFrom\fR function is used to calculate the position of the
 * next tab from a gixen \fIx\fR pixel using the TabTable \fItabs\fR.
 *
 * See also:
 *
 * _StringWidth(3), _CharWidth(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
extern int
_NextTabFrom(int           x,
	     TabTable      tabs,
	     int           maxwidth)
{
    if (maxwidth == 0)
    {
#ifdef DEBUG
	fprintf( stderr, "Warning:_NextTabFrom: 0 maxwidth, setting it to 10\n" );
#endif /* DEBUG */
	/* Setting maxwidth to 10: an arbitrary value */
	maxwidth = 10;
    }

    while(tabs != NULL && *tabs != 0 && (int)(*tabs) <= x)
	tabs++;
    if (tabs == NULL || *tabs == 0)
	x = ((x / (maxwidth * 8) + 1) * 8 * maxwidth) - x;
    else
	x = *tabs - x;
    return (x);
} /* end of _NextTabFrom */

/*
 * _DrawWrapLine
 *
 * The \fI_DrawWrapLine\fR function performs the actual drawing of the text.
 *
 * See also:
 *
 * _WrapLine(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
extern void
_DrawWrapLine(
	      int           x,
	      int           startx,
	      int           maxx,
	      OlStr         op,
	      UnitPosition  offset,
	      UnitPosition  len,
	      OlFont        fs,
	      TabTable      tabs,
	      OlWrapMode    mode,
	      Display *     dpy,
	      Window        win,
	      GC            normal,
	      GC            select,
	      int           y,
	      int           ascent,
	      int           fontht,
	      UnitPosition  current,
	      UnitPosition  start,
	      UnitPosition  end,
	      Pixel         bg,
	      Pixel         fg,
	      int           xoffset,
	      Boolean 	    sensitive,
	      PreEditRec*   pre_edit_ptr)
{
    XFontStruct * fstct      = (XFontStruct *)fs;
    char *        p          = (char *)op;
    char *        wordstart  = &p[offset];
    char *        wordend    = wordstart - 1;
    char *        maxp       = &p[len - 1];
    int           wordstartx = x;
    int           width      = 0;
    int           gctype     = (start != end && start < current && current <= end);
    int           recttop    = y - ascent;
    int           min        = fstct-> min_char_or_byte2;
    int           max        = fstct-> max_char_or_byte2;
    XCharStruct * per_char   = fstct-> per_char;
    int           maxwidth   = fstct-> max_bounds.width;
    int           c;
    int             (*drawFunc)(Display *display, Drawable d, GC gc,
                                    int x, int y, const char *string,
                                    int length);

    drawFunc = (sensitive ? XDrawImageString : XDrawString);

    if (wordstartx > startx) {
	width = wordstartx - startx;
	if (gctype)
	    XFillRectangle(dpy, win, normal, startx, recttop, width, fontht);
	else
	    XFillRectangle(dpy, win, select, startx, recttop, width, fontht);
	width = 0;
    }

    for (p = wordstart; p <= maxp && x < maxx; wordend = p++, current++) {
	if (start != end && (start == current || current == end)) {
	    if (start == current)
		start = -1;
	    else
		end = -1;
	    if (wordend >= wordstart)
		(*drawFunc)(dpy, win, gctype ? select:normal, wordstartx, y, 
				 wordstart, wordend - wordstart + 1);
	    wordstartx = x;
	    wordstart  = p;
	    gctype     = (end != -1);
	    p--; 
	    current--;
	} else {
	    switch(*p) {
	    case '\t':
		width = _NextTabFrom(x - startx - xoffset, tabs, maxwidth);
		if (wordend >= wordstart)
		    (*drawFunc)(dpy, win, gctype ? select:normal, wordstartx, y,
				     wordstart, wordend - wordstart + 1);
		XFillRectangle(dpy, win, gctype ? normal:select, x, recttop, width, fontht);
		wordstartx = x + width;
		wordstart = p + 1;
		break;
	    case '\0':
		width = maxx;
		break;
	    default:
		width = _CharWidth(*p, fstct, per_char, min, max, maxwidth);
		if ((*IsNonPrintableChar)(*p)) {
		    char buffer[2];
		    buffer[0] = '^';
		    buffer[1] = *p + '@';
              
		    if (wordend >= wordstart)
			(*drawFunc)(dpy, win, gctype ? select:normal, 
					 wordstartx, y, wordstart, wordend - wordstart + 1);
		    (*drawFunc)(dpy, win, gctype ? select:normal, 
				     x, y, buffer, 2);
		    wordstartx = x + width;
		    wordstart = p + 1;
		}
	    }
	    x += width;
	}
    }

    if (x > maxx) {
	x -= width;
	wordend--;
    }

    if ((width = wordend - wordstart + 1) > 0)
	(*drawFunc)
	    (dpy, win, gctype ? select:normal, wordstartx, y, wordstart, width);

    if ((width = maxx - (x = MAX(x, startx))) > 0)
	    XFillRectangle(dpy, win, select, x, recttop, width, fontht);

} /* end of DrawWrapLineSB */


/*
 * _DrawWrapLineMB
 *
 * The \fI_DrawWrapLine\fR function performs the actual drawing of the text.
 *
 * See also:
 *
 * _WrapLine(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
extern void
_DrawWrapLineMB(
	      int           x,
	      int           lmarg_x,
	      int           rmarg_x,
	      OlStr         op,
	      UnitPosition  offset,
	      UnitPosition  len,
	      OlFont        fs,
	      TabTable      tabs,
	      OlWrapMode    mode,
	      Display *     dpy,
	      Window        win,
	      GC            normal,
	      GC            select,
	      int           y,
	      int           ascent,
	      int           fontht,
	      UnitPosition  current,
	      UnitPosition  seln_start,
	      UnitPosition  seln_end,
	      Pixel         bg,
	      Pixel         fg,
	      int           xoffset,
	      Boolean	    sensitive,
	      PreEditRec*   pre_edit_ptr)
{
    XFontSet       fset       = (XFontSet)fs;
    int            tabwidth   = _GetmbTabWidth(fs); 
    int            maxheight   = (XExtentsOfFontSet(fs))->max_logical_extent.height;
    char *         mp         = (char *)op;
    char *         maxp       = &mp[len - 1];
    char *         stc        = &mp[offset];
    register char *left       = stc;
    register char *right;
    register char *p;
    int            left_x     = x;
    int            width      = 0;
    int            in_seln    = (seln_start != seln_end &&
				 seln_start < current   &&
				 current <= seln_end);
    GC             gc         = in_seln? select:normal;
    GC             igc        = in_seln? normal:select;
    int            recttop    = y - ascent;
    int            stlen;
    wchar_t        wch;
    int            clen       = mbtowc(&wch, stc, MB_CUR_MAX);
    int            lclen      = clen;
    int	   pindex = 0;	
    void           (*mbDrawFunc)(Display *display, Drawable d,
                                      XFontSet font_set, GC gc,
                                      int x, int y, const char *text,
                                      int bytes_text);

    if((pre_edit_ptr != NULL)                   &&
        (current > pre_edit_ptr->start)         &&
                (current < pre_edit_ptr->end)){
        char *ptr = (pre_edit_ptr->start > (current - offset)?
                        &mp[pre_edit_ptr->start -(current - offset)]: mp);

        for(pindex=0; ptr < stc; ptr += mblen(ptr, MB_CUR_MAX))
                                        pindex++;
    }

    mbDrawFunc = (sensitive ? XmbDrawImageString : XmbDrawString);

    XFillRectangle(dpy,win,select,lmarg_x,recttop,
			(unsigned int)(rmarg_x -lmarg_x),(unsigned int)fontht);
    if(mode == OL_WRAP_OFF)
	XClearArea(dpy,win,rmarg_x,recttop,(unsigned int)0, fontht, FALSE);

    for (p = stc, right = left - lclen;
	 p <= maxp && x < rmarg_x && (clen = mbtowc(&wch, p, MB_CUR_MAX)) > 0;
	 right =  p += clen, current+=clen)
    {
	if(pre_edit_ptr != NULL &&
		current >= pre_edit_ptr->start && 
				current < pre_edit_ptr->end) { 

		if(current == pre_edit_ptr->start) {
	    		if ((stlen = right - left) > 0)
				(*mbDrawFunc)(dpy, win, fset, gc, left_x, y, 
									left, stlen);
			left = p;
			left_x = x;
		}  

		    switch((int)pre_edit_ptr->pre_edit_feedback[pindex++]) {
			case XIMReverse:
			case XIMPrimary:
			case XIMHighlight:
			case XIMTertiary:
				(*mbDrawFunc)(dpy,win,fset,
				pre_edit_ptr->feedbackGCs[0],left_x,y,left,clen);
				break;
			case XIMUnderline:
			case XIMSecondary:
				width = _CharWidthWC(wch, fset);
				(*mbDrawFunc)(dpy,win,fset,
					pre_edit_ptr->feedbackGCs[1],left_x,y,left,clen);
				XDrawLine(dpy,win,pre_edit_ptr->feedbackGCs[1],
					    x,       y+(fontht-ascent)-1,
					    x+width, y+(fontht-ascent)-1);
				break;
			default:
				(*mbDrawFunc)(dpy,win,fset,gc,left_x,y,left,clen);
				break;
		     } /* switch */
		     
		width = _CharWidthWC(wch, fset);
		x += width;
		left_x = x;
		left += clen;
		continue; /* for loop */
	}  
		
		
	if (seln_start != seln_end  && (seln_start == current || current == seln_end)) {
	    if ((stlen = right - left) > 0)
			(*mbDrawFunc)(dpy, win, fset, gc, left_x, y, 
							left, stlen);
	    if (seln_start == current) {
		seln_start = -1;
		gc         = select;
		igc        = normal;
		left       = p;	
	    } else {
		seln_end = -1;
		gc       = normal;
		igc      = select;
		left     = p;
	    }
		left_x = x;
		p -= clen;
		current -= clen;
	} else {
	    if (wch == L'\t') {
		    if ((stlen = right - left) > 0) {
			(*mbDrawFunc)(dpy, win, fset, gc, left_x, y,
					   			left, stlen);
		    }
		    width = _NextTabFrom(x - lmarg_x - xoffset, tabs, tabwidth);
		    XFillRectangle(dpy, win, igc, x, recttop, width, fontht);
		    left_x = x + width;
		    left = p + clen;
	    } else 
		width = _CharWidthWC(wch, fset);
	    
	    x += width;
	}
	lclen = clen;
    }

    if (x > rmarg_x) {
	x     -= width;
        if ((stlen = right - left) > 0)
	    (*mbDrawFunc)(dpy, win, fs, gc, left_x, y, left, stlen - lclen);
    } else if ((stlen = right - left) > 0)
	    (*mbDrawFunc)(dpy, win, fs, gc, left_x, y, left, stlen);
} /* end of DrawWrapLineMB */


/*
 * _DrawWrapLineWC
 *
 * The \fI_DrawWrapLineWC\fR function performs the actual drawing of wide-char text.
 *
 * See also:
 *
 * _WrapLine(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
extern void
_DrawWrapLineWC(
	      int           x,
	      int           lmarg_x,
	      int           rmarg_x,
	      OlStr         op,
	      UnitPosition  offset,
	      UnitPosition  len,
	      OlFont        fs,
	      TabTable      tabs,
	      OlWrapMode    mode,
	      Display *     dpy,
	      Window        win,
	      GC            normal,
	      GC            select,
	      int           y,
	      int           ascent,
	      int           fontht,
	      UnitPosition  current,
	      UnitPosition  seln_start,
	      UnitPosition  seln_end,
	      Pixel         bg,
	      Pixel         fg,
	      int           xoffset,
	      Boolean	    sensitive,
	      PreEditRec    *pre_edit_ptr)
{
    XFontSet           fset       = (XFontSet)fs;
    int                tabwidth   = _GetwcTabWidth(fs); 
    wchar_t *          mp         = (wchar_t *)op;
    wchar_t *          maxp       = &mp[len - 1];
    wchar_t *          stc        = &mp[offset];
    register wchar_t * left       = stc;
    register wchar_t * right;
    register wchar_t * p;
    int                left_x     = x;
    int                width      = 0;
    int                in_seln    = (seln_start != seln_end &&
				     seln_start < current   &&
				     current <= seln_end);
    GC                 gc         = in_seln? select:normal;
    GC                 igc        = in_seln? normal:select;
    int                recttop    = y - ascent;
    int                stlen;
    wchar_t            wch;
    int 		pindex = 0;
    void               (*wcDrawFunc)(Display *display, Drawable d,
                                      XFontSet font_set, GC gc,
                                      int x, int y, wchar_t *text,
                                      int num_wchars);

    if((pre_edit_ptr != NULL)                   &&
        (current > pre_edit_ptr->start)         &&
                (current < pre_edit_ptr->end))
                pindex = current - (pre_edit_ptr->start > (current -offset)?
                                pre_edit_ptr->start: (current - offset));

    wcDrawFunc = (sensitive ? XwcDrawImageString : XwcDrawString);

    XFillRectangle(dpy,win,select,lmarg_x,recttop,
			(unsigned int)(rmarg_x -lmarg_x),(unsigned int)fontht);
    if(mode == OL_WRAP_OFF)
	XClearArea(dpy,win,rmarg_x,recttop,(unsigned int)0, fontht, FALSE);

    for (p = stc, right = left - 1;
	 p <= maxp && x < rmarg_x && *p != L'\0';
				 right = ++p, current++) {

	if(pre_edit_ptr != NULL &&
		current >= pre_edit_ptr->start && 
				current < pre_edit_ptr->end) { 
               
		wch = *p;

		if(current == pre_edit_ptr->start) {
	    		if ((stlen = right - left) > 0)
				(*wcDrawFunc)(dpy, win, fset, gc, left_x, y, 
									left, stlen);
			left = p;
			left_x = x;
		} 

		   switch((int)pre_edit_ptr->pre_edit_feedback[pindex++]) {
			case XIMReverse:
			case XIMPrimary:
			case XIMHighlight:
			case XIMTertiary:
				(*wcDrawFunc)(dpy,win,fset,
				pre_edit_ptr->feedbackGCs[0],left_x,y,left,1);
				break;
			case XIMUnderline:
			case XIMSecondary:
				width = _CharWidthWC(wch, fset);
				(*wcDrawFunc)(dpy,win,fset,
					pre_edit_ptr->feedbackGCs[1],left_x,y,left,1);
				XDrawLine(dpy,win,pre_edit_ptr->feedbackGCs[1],
					    x,       y+(fontht-ascent)-1,
					    x+width, y+(fontht-ascent)-1);
				break;
			default:
				(*wcDrawFunc)(dpy,win,fset,gc,left_x,y,left,1);
				break;
		    } /* switch */

		width = _CharWidthWC(wch, fset);
		x += width;
		left_x = x;
		left++;
		continue; /* for loop */
	}

	if (seln_start !=  seln_end  && (seln_start == current || current == seln_end)) {
	    if ((stlen = right - left) > 0)
		(*wcDrawFunc)(dpy, win, fset, gc, left_x, y, 
							left, stlen );
	    if (seln_start == current) {
		seln_start = -1;
		gc         = select;
		igc        = normal;
		left       = p;	/* right must be less than left */
	    } else {
		seln_end = -1;
		gc       = normal;
		igc      = select;
		left     = p; /* right must be less than left */
	    }
	    left_x     = x;
	    p--;       /* back up, so that the else can now process it */
	    current--;
	} else {
	    wch = *p;
	    if (wch == L'\t') {
		    if ((stlen = right - left) > 0) {
			(*wcDrawFunc)(dpy, win, fset, gc, left_x, y,
					   left, stlen);
		    }
		    width = _NextTabFrom(x - lmarg_x - xoffset, tabs, tabwidth);
		    XFillRectangle(dpy, win, igc, x, recttop, width, fontht);
		    left_x = x + width;
		    left = p + 1;
	    } else 
		width = _CharWidthWC(wch, fset);

	    x += width;
	}
    }

    if (x > rmarg_x) {
	right --;
	x     -= width;
    }
    if ((stlen = right - left) > 0)
	(*wcDrawFunc)(dpy, win, fs, gc, left_x, y, left, stlen);

} /* end of DrawWrapLineWC */

/*
 * _WrapLine
 *
 * The \fI_WrapLine\fR function calculates the next wrap position offset 
 * in the string \fIp\fR from the given \fIoffset\fR.  The calculation
 * begins from the \fIx\fR pixel through the \fImaxx\fR pixel and uses
 * the XFontstruct pointer \fIfs\fR and TabTable \fItabs\fR.  The type
 * of wrap to be performed is specified in \fImode\fR.
 *
 * See also:
 *
 * _DrawWrapLine(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
extern TextPosition
_WrapLine(int           x,
	  int           maxx,
	  OlStr         p,
	  TextPosition  offset,
	  UnitPosition *uoffset,
	  OlFont        olfs,
	  TabTable      tabs, 
	  OlWrapMode    mode)
{
    int           startx    = x;
    char *        ss        = (char *)p;
    char *        wordend   = &ss[offset];
    char *        savep;
    XFontStruct * fs        = (XFontStruct *)olfs;
    int           min       = fs-> min_char_or_byte2;
    int           max       = fs-> max_char_or_byte2;
    XCharStruct * per_char  = fs-> per_char;
    int           maxwidth  = fs-> max_bounds.width;
    int           c;

    for (savep = ss = wordend; x <= maxx && *ss; ss++) {
	switch(*ss) {
	case '\t':
	    x += _NextTabFrom(x - startx, tabs, maxwidth);
	    wordend = (x < maxx ? ss  : wordend);
	    break;
	case ' ':
	    x += _CharWidth(*ss, fs, per_char, min, max, maxwidth);
	    wordend = (x < maxx ? ss  : wordend);
	    break;
	default:
	    x += _CharWidth(*ss, fs, per_char, min, max, maxwidth);
	    break;
	}
    }
    if (x > maxx || *ss)
	if (mode == OL_WRAP_WHITE_SPACE && wordend != savep)
	    offset += ((wordend - savep) + 1);
	else
	    offset += (ss - savep - 1);
    else
	offset = EOF;
    *uoffset = offset;
    return(offset);
} /* end of _WrapLine */

/*
 * _WrapLineMB
 *
 * The \fI_WrapLineMB\fR function calculates the next wrap position offset 
 * in the string \fIp\fR from the given \fIoffset\fR.  The calculation
 * begins from the \fIx\fR pixel through the \fIrmarg_x\fR pixel and uses
 * the XFontSet pointer \fIfs\fR and TabTable \fItabs\fR.  The type
 * of wrap to be performed is specified in \fImode\fR.
 *
 * See also:
 *
 * _DrawWrapLine(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
extern TextPosition
_WrapLineMB(int           x,
	    int           rmarg_x,
	    OlStr         p,
	    TextPosition  offset,
	    UnitPosition *uoffset,
	    OlFont        olfs,
	    TabTable      tabs, 
	    OlWrapMode    mode)
{
    int           left_x    = x;
    char *        ms        = (char *)p;
    register char *sms;
    register char *wms      = &ms[*uoffset];
    XFontSet      fs        = (XFontSet)olfs;
    int           tabwidth  = _GetmbTabWidth(fs);
    wchar_t       wch;
    int           clen;
    register int  i;
    int           wi;
    char 	  str[2];
    int		  count	    = 0;

	if (x > rmarg_x) 
		return EOF;

    for (wi = i = 0, sms = ms = wms;
	 x <= rmarg_x && (clen = mbtowc(&wch, ms, MB_CUR_MAX)) > 0;
	 ms+=clen, i++)
    {
	if (!iswgraph(wch)) { /* instead of iswprint; to include space */
	    switch(wch) {
	    case L'\t':
		x += _NextTabFrom(x - left_x, tabs, tabwidth);
		wi = ( x < rmarg_x ? i : wi);
		wms = (x < rmarg_x ? ms : wms);
		break;
	    case L' ':
		x += _CharWidthWC(wch, fs);
		wi = ( x < rmarg_x ? i : wi);
		wms = (x < rmarg_x ? ms : wms);
		break;
	    default:
		x += _CharWidthWC(wch, fs);
		break;
	    }
	} else {
	    x += _CharWidthWC(wch, fs);
	}
    }
    if (x > rmarg_x || clen > 0)
	if (mode == OL_WRAP_WHITE_SPACE && wi) {
	    offset += wi + 1;
	    *uoffset += (wms - sms + 1); /* 
					  *we add 1 'cause ' ' is 
					  * always 1 byte long and we
					  * want the first char after space. 
					  */
	} else {
	    offset += (i > 1 ? i - 1: 1);
	    *uoffset += (ms - sms  > clen ? ms - sms -clen : ms -sms);
	}
    else
	offset = EOF;
    return(offset);
} /* end of _WrapLineMB */

/*
 * _WrapLineWC
 *
 * The \fI_WrapLineWC\fR function calculates the next wrap position offset 
 * in the string \fIp\fR from the given \fIoffset\fR.  The calculation
 * begins from the \fIx\fR pixel through the \fIrmarg_x\fR pixel and uses
 * the XFontSet pointer \fIfs\fR and TabTable \fItabs\fR.  The type
 * of wrap to be performed is specified in \fImode\fR.
 *
 * See also:
 *
 * _DrawWrapLine(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
extern TextPosition
_WrapLineWC(int           x,
	    int           rmarg_x,
	    OlStr         p,
	    TextPosition  offset,
	    UnitPosition *uoffset,
	    OlFont        olfs,
	    TabTable      tabs, 
	    OlWrapMode    mode)
{
    int           left_x    = x;
    wchar_t *     iws       = (wchar_t *)p;
    wchar_t *     ws        = &iws[*uoffset];
    XFontSet      fs        = (XFontSet)olfs;
    int           tabwidth  = _GetwcTabWidth(fs); 
    wchar_t       wch;
    register int  i;
    int           wi;

	if (x > rmarg_x) 
		return EOF;
    
    for (wi = i = 0;
	 x <= rmarg_x && (wch = *ws) != L'\0';
	 ws++, i++)
    {
	if (!iswgraph(wch)) { /* instead of iswprint; to include space */
	    switch(wch) {
	    case L'\t':
		x += _NextTabFrom(x - left_x, tabs, tabwidth);
		wi = (x < rmarg_x ? i: wi);
		break;
	    case L' ':
		x += _CharWidthWC(wch, fs);
		wi = (x < rmarg_x ? i: wi);
		break;
	    default:
		x += _CharWidthWC(wch, fs);
		break;
	    }
	} else {
	    x += _CharWidthWC(wch, fs);
	}
    }
    if (x > rmarg_x || wch != L'\0')
	if (mode == OL_WRAP_WHITE_SPACE && wi)
	    offset += wi + 1;
	else
	    offset += (i > 1 ? i - 1: 1);
    else
	offset = EOF;
    *uoffset = offset;
    return(offset);
} /* end of _WrapLineWC */

/*
 * OlTextEditSetNonPrintableCharFunc
 *
 * The \fIOlTextEditSetNonPrintableCharFunc\fR procedure provides the 
 * capability to replace the non_printable char definition function used by the 
 * TextEdit Utilities.  This function is called as:~
 *
 * .so CWstart
 * 	(*non_printable_char_definition)(c);
 * .so CWend
 *
 * The function is responsible for returning non-zero if the character
 * c is considered a non_printable character and zero
 * otherwise.
 *
 * Calling this function with NULL reinstates the default non_printable
 * character definition
 * which allows the ISO-Latin definition of non-printable.
 *
 * Synopsis:
 *
 * #include <TextEdit.h>
 *  ...
 */
void
OlTextEditSetNonPrintableCharFunc(_OlNonPrintableCharFunc non_printable_char_func)
{

    IsNonPrintableChar = non_printable_char_func ?
	non_printable_char_func : is_non_printable_char;

} /* end of OlTextEditSetNonPrintableCharFunc */

/*
 * is_non_printable_char
 *
 */
static int
is_non_printable_char(int rc)
{
	unsigned char		c = (unsigned char) rc;
	int			retval = (0 < c && c < 040 || 0177 < c && 
					c < 0240);

    return (retval);

} /* end of is_non_printable_char */

/*
 * Function to increment/decrement wrap location using text position
 * offset instead of number of wrap lines. This has been added for uniform
 * scrolling even when wrapTable is partially computed.
 */
extern TextLocation
_IncrementWrapLocByTextDelta(TextEditWidget	ctx,
			     TextLocation	current,
			     int *		text_delta,
			     int *		reallines,
			     Boolean		atleast_1line)
{
    TextEditPart * text            = &ctx->textedit;
    WrapTable *    wrapTable       = text->wrapTable;
    WrapContents * wrc             = wrapTable->contents;
    TextBuffer *   textBuffer      = text->textBuffer;
    OlStrRep       rep             = text->text_format;
    int		   n		   = 0;
    int		   line_chars;
    int		   i		   = 0;
    Boolean	   more_lines	   = TRUE;
    Boolean	   wrap_updated    = FALSE;

#ifdef DEBUG_TEXTWRAP
    fprintf(stderr, "Input delta: %d\n", *text_delta);
#endif	/* DEBUG_TEXTWRAP */
    
    if (*text_delta < 0) {
	TextLocation	top_line = _FirstWrapLine(wrapTable);
	int		chars_decr;

	if (current.line < top_line.line)
	    if (top_line.line > 0) {
		_UpdateWrapTableAbove(ctx, True);
		top_line = _FirstWrapLine(wrapTable);
		wrap_updated = TRUE;
	    } else
		more_lines = FALSE;
	if (more_lines)
	    do {
		line_chars = wrc->p[current.line]->p[current.offset];
		if (--current.offset < 0) {
		    if (!atleast_1line && (n-line_chars) < *text_delta) {
			current.offset++;
			break;
		    }
		    n             -= line_chars;
		    if (--current.line < top_line.line)
			if (top_line.line > 0) {
			    _UpdateWrapTableAbove(ctx, True);
			    top_line = _FirstWrapLine(wrapTable);
			    wrap_updated = TRUE;
			} else {
			    current.line++;
			    current.offset++;
			    break;
			}
		    current.offset = wrc->p[current.line]->used - 1;
		    if (rep == OL_SB_STR_REP)
			line_chars = LastCharacterInTextBufferLine(textBuffer,
						   current.line) + 1;
		    else
			line_chars =
			    OlNumCharsInTextBufferLine(
			       (OlTextBufferPtr) textBuffer, current.line);
		}
		chars_decr  = line_chars -
		    wrc->p[current.line]->p[current.offset];
		if (!atleast_1line && (n - chars_decr) < *text_delta) {
		    break;
		}
		n          -= chars_decr;
		line_chars  = wrc->p[current.line]->p[current.offset];
		i--;
	    } while (n > *text_delta);

    } else if (*text_delta > 0) {
	TextLocation	bot_line = _LastWrapLine(wrapTable);
	int		old_offs = 0;
	int		chars_incr;
	
	if (current.line > bot_line.line)
	    if (bot_line.line+1 < wrc->used) {
		_UpdateWrapTableBelow(ctx, True);
		bot_line = _LastWrapLine(wrapTable);
		wrap_updated = TRUE;
	    } else
		more_lines = FALSE;
	if (more_lines) {
	    if (rep == OL_SB_STR_REP)
		line_chars = LastCharacterInTextBufferLine(textBuffer,
							   current.line) + 1;
	    else
		line_chars =
		    OlNumCharsInTextBufferLine((OlTextBufferPtr) textBuffer,
					       current.line);
	    if (current.offset > 0) {
		line_chars -= wrc->p[current.line]->p[current.offset];
		old_offs    = wrc->p[current.line]->p[current.offset];
	    }
	    do {
		if (++current.offset >
		    wrapTable->contents->p[current.line]->used - 1)
		{
		    if (!atleast_1line && (n + line_chars) > *text_delta) {
			current.offset--;
			break;
		    }
		    old_offs       = 0;
		    n             += line_chars;
		    if (++current.line > bot_line.line)
			if (bot_line.line+1 < wrc->used) {
			    _UpdateWrapTableBelow(ctx, True);
			    bot_line = _LastWrapLine(wrapTable);
			    wrap_updated = TRUE;
			} else {
			    current.line--;
			    current.offset--;
			    break;
			}
		    current.offset = 0;
		    if (rep == OL_SB_STR_REP)
			line_chars = LastCharacterInTextBufferLine(textBuffer,
							   current.line) + 1;
		    else
			line_chars =
			    OlNumCharsInTextBufferLine(
				       (OlTextBufferPtr) textBuffer,
				       current.line);
		} else {
		    chars_incr  = wrc->p[current.line]->p[current.offset]
			- old_offs;
		    old_offs    = wrc->p[current.line]->p[current.offset];
		    if (!atleast_1line && n + (chars_incr) > *text_delta) {
			current.offset--;
			break;
		    }
		    n          += chars_incr;
		    line_chars -= chars_incr;
		}
		i++;
	    } while (n < *text_delta);
	}
    }
    *text_delta = n;
    if (reallines != (int *) NULL)
	*reallines = i;
    if (wrap_updated)
	(void) _BGDisplayChange(ctx);
#ifdef DEBUG_TEXTWRAP
    fprintf(stderr, "Output delta: %d\n", *text_delta);
#endif	/* DEBUG_TEXTWRAP */
    return(current);
}

/*
 *  _GetmbTabWidth
 *
 */

static int _GetmbTabWidth( XFontSet fs )
{
    int tabwidth = 0;

    tabwidth = XmbTextEscapement(fs, "m", 1);
    if ( tabwidth == 0 )
    {
#ifdef DEBUG
    fprintf(stderr, "_GetmbTabWidth: fontset doesn't have char. m, tabwidth is 0 \n");
#endif /* DEBUG */
	tabwidth = (XExtentsOfFontSet(fs))->max_logical_extent.width;
    }

#ifdef DEBUG
    if (tabwidth == 0)
       fprintf(stderr, 
	   "_GetmbTabWidth: fontset doesn't have max width, tabwidth is 0 \n");
#endif /* DEBUG */
    return tabwidth;
}

/*
 *  _GetwcTabWidth
 *
 */
static int _GetwcTabWidth( XFontSet fs )
{
    int tabwidth = 0;

    tabwidth = XwcTextEscapement(fs, L"m", 1);

    if ( tabwidth == 0 )
    {
#ifdef DEBUG
    fprintf(stderr, "_GetwcTabWidth: fontset doesn't have char. m, tabwidth is 0 \n");
#endif /* DEBUG */
	tabwidth = (XExtentsOfFontSet(fs))->max_logical_extent.width;
    }

#ifdef DEBUG
    if (tabwidth == 0)
	fprintf(stderr, 
	     "_GetwcTabWidth: fontset doesn't have max width, tabwidth is 0 \n");
#endif /* DEBUG */
    return tabwidth;
}
