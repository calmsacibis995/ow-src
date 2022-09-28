#pragma ident   "@(#)TextDisp.c	302.32    97/03/26 lib/libXol SMI"     /* textedit:TextDisp.c 1.14
*/
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


#include <string.h>

#include <X11/IntrinsicP.h>

#include <Xol/Dynamic.h>
#include <Xol/Oltextbuff.h>
#include <Xol/OpenLookP.h>
#include <Xol/Scrollbar.h>
#include <Xol/TextDisp.h>
#include <Xol/TextEPos.h>
#include <Xol/TextEditP.h>
#include <Xol/TextUtil.h>
#include <Xol/TextWrap.h>
#include <Xol/Util.h>
#include <Xol/buffutil.h>
#include <Xol/memutil.h>
#include <Xol/strutil.h>
#include <Xol/textbuff.h>
#include <Xol/OlIm.h>

#define unsigned
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/smallIn.h>
#include <Xol/bitmaps/smallOut.h>
#include <Xol/bitmaps/mediumIn.h>
#include <Xol/bitmaps/mediumOut.h>
#include <Xol/bitmaps/largeIn.h>
#include <Xol/bitmaps/largeOut.h>
#include <Xol/bitmaps/hugeIn.h>
#include <Xol/bitmaps/hugeOut.h>
#include <Xol/bitmaps/pt16CurIn.h>
#include <Xol/bitmaps/pt16CurOut.h>
#include <Xol/bitmaps/biggestCurIn.h>
#include <Xol/bitmaps/biggestCurOut.h>
#undef char
#undef unsigned


#define HAS_FOCUS(w)	(((TextEditWidget)(w))->primitive.has_focus)
#define ACTIVE_CARET(w)	(HAS_FOCUS(w) && ((((TextEditWidget)(w))->textedit.editMode == OL_TEXT_EDIT)))


static void		CalculateDisplayTable(TextEditWidget ctx);
static XRectangle*	CalculateRectangle(TextEditWidget ctx);
static void		_BlinkCursor(XtPointer client_data, XtIntervalId *id);


static void
CalculateDisplayTable(TextEditWidget ctx)
{
    TextEditPart * text       = &(ctx-> textedit);
    WrapTable *    wrapTable  = text-> wrapTable;
    TextBuffer *   textBuffer = text-> textBuffer;
    Boolean pre_edit_stuff_exists = FALSE;

    DisplayTable * DT;
    char *         p;
    register int   i;
    TextLocation   wrapline;
    TextPosition   wrap_pos;

    if(text->text_format != OL_SB_STR_REP &&
		text->pre_edit_end  > text->pre_edit_start) 
	pre_edit_stuff_exists = TRUE;
    

    if (text-> DT == NULL || text-> DTsize < text-> linesVisible) {
	if (text-> DTsize == 0) {
	    text-> DT = (DisplayTable *)
		XtMalloc(text-> linesVisible*sizeof(DisplayTable));
	} else {
	    text-> DT = (DisplayTable *)
		XtRealloc((char *)text-> DT,
			sizeof(DisplayTable) * text-> linesVisible);
	}
    }
    DT = text-> DT;
    for (i = 0; i < text-> DTsize; i++) {
	if (DT[i].p) 
	    XtFree(DT[i].p);
	if(DT[i].pre_edit_feedback)
	    XtFree((char *)DT[i].pre_edit_feedback);
	text->DT[i].p = text->DT[i].pre_edit_feedback = NULL;
    }

    text-> DTsize = text-> linesVisible;
    wrapline = _WrapLocationOfLocation(wrapTable, text-> displayLocation);
    for (i = 0; i < text-> linesVisible; i++) {
	DT[i].wraploc = wrapline;
	if ((p = _GetNextWrappedLine(textBuffer, wrapTable, &wrapline)) == NULL) {
	    for (; i < text-> linesVisible; i++) {
		DT[i].nchars =
		DT[i].feedback_start_offset =
		DT[i].num_feedback_chars =
		DT[i].used = 
		    DT[i].wraploc.line = DT[i].wraploc.offset = 0;
		DT[i].pre_edit_feedback = (XIMFeedback *)NULL;
		DT[i].p = NULL;
	    }
	} else {
	    DT[i].used = _WrapLineNumUnits(textBuffer, wrapTable, DT[i].wraploc);
	    DT[i].nchars = _WrapLineLength(textBuffer, wrapTable, DT[i].wraploc);
	    if (ctx->textedit.text_format == OL_WC_STR_REP) {
		wchar_t *wp = NULL;
		int l = DT[i].used * sizeof(wchar_t);
		
		DT[i].p = memcpy(XtMalloc(l+sizeof(wchar_t)), p, l);
		wp = (wchar_t *)DT[i].p;
		wp[DT[i].used] = L'\0';
	    } else {
		DT[i].p = memcpy(XtMalloc(DT[i].used + 1), p, DT[i].used);
		((unsigned char*)(DT[i].p))[DT[i].used] = '\0';
	    }

	    wrap_pos = _PositionOfWrapLocation(textBuffer, wrapTable,DT[i].wraploc);
	    if(pre_edit_stuff_exists == TRUE && 
		wrap_pos < text->pre_edit_end &&
			text->pre_edit_start < wrap_pos + DT[i].nchars) {

		/* feedback_start_pos and feedback_end_pos give the start and
		 * end position of feedback in the current wrap line */

		int feedback_end_pos = MIN((DT[i].nchars + wrap_pos),text->pre_edit_end);
		int feedback_start_pos = MAX(wrap_pos, text->pre_edit_start);
		int num_feedback_chars = feedback_end_pos - feedback_start_pos;

		DT[i].pre_edit_feedback = 
			(XIMFeedback *)XtMalloc(num_feedback_chars*sizeof(XIMFeedback));	
		memmove((void *)DT[i].pre_edit_feedback,
			(void *)&text->pre_edit_feedback->p[feedback_start_pos -
					text->pre_edit_start],
					num_feedback_chars*sizeof(XIMFeedback));
		DT[i].feedback_start_offset = feedback_start_pos - wrap_pos;
		DT[i].num_feedback_chars = num_feedback_chars;
	    } else {
		DT[i].feedback_start_offset =
		DT[i].num_feedback_chars = 0;
		DT[i].pre_edit_feedback = (XIMFeedback *)NULL;
	   }
	 }
    }
} /* end of CalculateDisplayTable */

/*
 * CalculateRectangle
 *
 */
typedef UnitPosition (*StrMatchFunc)(OlStr, OlStr, int, int*);
	/* 
	 * returns unit offset of last matching character. 
	 * If the string differs from offset 0, it
	 * returns -1.In the fourth argument,
	 * it returns the char offset information
	 * with the same semantics as in the case of unit offset.
	 */

static UnitPosition sb_strnmatch(OlStr, OlStr, int, int*);
static UnitPosition mb_strnmatch(OlStr, OlStr, int, int*);
static UnitPosition wc_strnmatch(OlStr, OlStr, int, int*);

static StrMatchFunc str_match[NUM_SUPPORTED_REPS] = {
    sb_strnmatch,
    mb_strnmatch,
    wc_strnmatch,
};

static XRectangle *
CalculateRectangle(TextEditWidget ctx)
{
    TextEditPart * text       = &(ctx->textedit);
    WrapTable *    wrapTable  = text->wrapTable;
    TextBuffer *   textBuffer = text->textBuffer;
    OlFont         fs         = ctx->primitive.font;
    TabTable       tabs       = text->tabs;
    int fontht                = text->lineHeight;

    DisplayTable * DT;
    char *         p;
    register int   i;
    TextLocation   wrapline;
    int            y;
    int            n;
    int            m;
    int            x;
    static XIMFeedbackBuffer *pre_edit_feedback = NULL;
    static XRectangle rect;
    Boolean pre_edit_stuff_exists = FALSE;

    if(text->text_format != OL_SB_STR_REP &&
		text->pre_edit_end  > text->pre_edit_start) {
	if(pre_edit_feedback == (XIMFeedbackBuffer *)NULL)
		pre_edit_feedback = (XIMFeedbackBuffer *)
				AllocateBuffer(sizeof(XIMFeedback),0);
	pre_edit_stuff_exists = TRUE;
    }

    if ((DT = text-> DT) == NULL) {
	rect.x      =
	rect.y      = 0;
	rect.width  = ctx-> core.width;
	rect.height = ctx-> core.height;
	CalculateDisplayTable(ctx);
    } else {
	OlStrRep     rep       = text->text_format;
	StrMatchFunc strnmatch = str_match[rep];
	
	rect.x      =
	rect.y      =
	rect.width  =
	rect.height = 0;
	wrapline = _WrapLocationOfLocation(wrapTable, text-> displayLocation);
	y = PAGE_T_MARGIN(ctx);
	for (i = 0; i < text-> linesVisible; i++) {
	    DT[i].wraploc = wrapline;
	    if ((p = _GetNextWrappedLine(textBuffer, wrapTable, &wrapline)) == NULL) {
		Boolean prev_lines = FALSE;

		if (rect.y == 0)
		    rect.y = y	/* REMOVE - fontht*/;
		for (; i < text-> linesVisible; i++) {
		    DT[i].nchars = 
		    DT[i].feedback_start_offset = 
		    DT[i].num_feedback_chars = 
		    DT[i].used           =
		    DT[i].wraploc.line   =
		    DT[i].wraploc.offset = 0;
		    if (DT[i].p) {
			prev_lines = TRUE;
			XtFree(DT[i].p);
			DT[i].p = NULL;
		    }

		    if(DT[i].pre_edit_feedback) {
			XtFree((XtPointer)(DT[i].pre_edit_feedback));
			DT[i].pre_edit_feedback = NULL;
		    }
		}

		if(prev_lines == TRUE)	{
			rect.x     = 0;
			rect.width = ctx-> core.width;
			rect.height = PAGE_B_MARGIN(ctx) - rect.y;
		}

	    } else {
			int index = 0;
			TextPosition wrap_pos;
			int feedback_and_chars_same_till; /* measured in char positions */
			int chars_same_till;
			int nchars;

		wrap_pos = _PositionOfWrapLocation(textBuffer, wrapTable,DT[i].wraploc);
		n = _WrapLineNumUnits(textBuffer, wrapTable, DT[i].wraploc);
		m = strnmatch(DT[i].p, p, MAX(n, DT[i].used), &chars_same_till);
		nchars = _WrapLineLength(textBuffer, wrapTable, DT[i].wraploc);

		if(pre_edit_stuff_exists == TRUE		&&  
			wrap_pos < text->pre_edit_end 		&&
			text->pre_edit_start < wrap_pos + nchars) 	{

			/* feedback_start_pos and feedback_end_pos give the start and
			 * end position of feedback in the current wrap line */

			int feedback_end_pos = MIN((nchars + wrap_pos),text->pre_edit_end);
			int feedback_start_pos = MAX(wrap_pos, text->pre_edit_start);
			int num_feedback_chars = feedback_end_pos - feedback_start_pos;

			if(pre_edit_feedback->size < num_feedback_chars)
				GrowBuffer((Buffer *)pre_edit_feedback,num_feedback_chars);

			for(index = feedback_start_pos, pre_edit_feedback->used = 0; 
							index < feedback_end_pos; index++)
				pre_edit_feedback->p[pre_edit_feedback->used++] =
					ctx->textedit.pre_edit_feedback->p[index - text->pre_edit_start];

			if(DT[i].pre_edit_feedback == NULL) /* previously, no feedback */ 
				feedback_and_chars_same_till = 
				     MIN(chars_same_till,feedback_start_pos - wrap_pos -1);
			else  {
					/* previously feedback is there, so is there now */
			    int	new_preedit_offset = feedback_start_pos - wrap_pos;

			    if (new_preedit_offset == DT[i].feedback_start_offset) {
				int nc = MIN(num_feedback_chars,DT[i].num_feedback_chars);
				for(index = 0;  index < nc; index++) 
					if(DT[i].pre_edit_feedback[index] !=
								pre_edit_feedback->p[index])
							break;
				index--;
				feedback_and_chars_same_till = 
					MIN(chars_same_till,feedback_start_pos + 
									index - wrap_pos);	
			    } else {
				int	first_changed =
					MIN(DT[i].feedback_start_offset, new_preedit_offset);

				feedback_and_chars_same_till = first_changed - 1;
			    }
			}
			
			if(DT[i].pre_edit_feedback != NULL)
				XtFree((XtPointer)DT[i].pre_edit_feedback);
			DT[i].pre_edit_feedback = 
				(XIMFeedback *)XtMalloc(num_feedback_chars*sizeof(XIMFeedback));	
			memmove((void *)DT[i].pre_edit_feedback,
					(void *)pre_edit_feedback->p,
						num_feedback_chars*sizeof(XIMFeedback));
			DT[i].feedback_start_offset = feedback_start_pos - wrap_pos;
			DT[i].num_feedback_chars = num_feedback_chars;

		} else  {
				if(DT[i].pre_edit_feedback != NULL) {   
				/* no feedback now, previously some feedback */
					feedback_and_chars_same_till = MIN(chars_same_till,
								DT[i].feedback_start_offset - 1);  
                                	XtFree((XtPointer)DT[i].pre_edit_feedback); 
				} else
					feedback_and_chars_same_till = chars_same_till;

				DT[i].pre_edit_feedback = NULL;
				DT[i].feedback_start_offset = 0;
				DT[i].num_feedback_chars = 0;
		}

		if(feedback_and_chars_same_till > 0  &&
			text->text_format != OL_SB_STR_REP) {
			TextLocation loc;
			UnitPosition left;

			m = OlUnitPositionOfTextPosition((OlTextBufferPtr)textBuffer,
						wrap_pos + feedback_and_chars_same_till);	
			m -= OlUnitPositionOfTextPosition((OlTextBufferPtr)textBuffer,
									wrap_pos);
		} else 
			m = feedback_and_chars_same_till;
		

		if (DT[i].p == (OlStr)NULL 		|| 
			!((feedback_and_chars_same_till 
			== DT[i].nchars - 1) && 
			 (DT[i].nchars == nchars)) ) {
		    if (rect.y != 0) {
			rect.height = y - rect.y + fontht;
			rect.x      = 0;
			rect.width  = ctx-> core.width;
		    } else {
			rect.y      = y;
			rect.height = fontht;
			x           = (m >= 0 ) ?
					(_StringWidth(0, p, 0, m, fs, rep, tabs)
			              	+ text->xOffset) : 0 ;
			rect.x      = x + PAGE_L_MARGIN(ctx);
			rect.width  = ctx-> core.width - rect.x;
		    }
		    DT[i].used = n;
		    DT[i].nchars = nchars;
		    if (DT[i].p)
			XtFree(DT[i].p);
		    if (rep == OL_WC_STR_REP) {
			int l = DT[i].used * sizeof(wchar_t);
		
			DT[i].p = memcpy(XtMalloc(l+sizeof(wchar_t)), p, l);
			(void)memset((void*)&(((unsigned char*)(DT[i].p))[l]), 0, sizeof(wchar_t));
		    } else {
			DT[i].p = memcpy(XtMalloc(DT[i].used + 1), p, DT[i].used);
			((unsigned char*)(DT[i].p))[DT[i].used] = '\0';
		    }
		}
	    }
	    y += fontht;
	}
    }
    return (&rect);
} /* end of CalculateRectangle */

/*
 * _DisplayText
 *
 */
/* To enable fast switching of Draw routines. With its humongous # of args, */
/* it will slow down considerably if we put a wrapper function around it    */
typedef void (*WrapDrawFunc)(int           x,
			     int           startx,
			     int           maxx,
			     OlStr         p,
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
			     Boolean	   sensitive,
			     PreEditRec    *pre_edit_ptr);

static WrapDrawFunc  wrap_draw[NUM_SUPPORTED_REPS] = {
    _DrawWrapLine,
    _DrawWrapLineMB,
    _DrawWrapLineWC
    };

extern void
_DisplayText(TextEditWidget ctx,
	     XRectangle *   rect)
{
    TextEditPart *  text       = &(ctx->textedit);
    OlStrRep        rep        = text->text_format;
    OlFont          fs         = ctx->primitive.font;
    WrapTable *     wrapTable  = text->wrapTable;
    TextBuffer *    textBuffer = text->textBuffer;
    int             ascent = ASCENT(ctx);
    int             fontht     = text->lineHeight;
    TabTable        tabs       = text->tabs;
    DisplayTable *  DT;
    int             start;
    int             end;
    Boolean         sensitive;
    register int    i;
    register int    y;
    register OlStr  p;
    TextLocation    insline;
    TextLocation    prevline;
    int             diff;
    TextPosition    wrappos;
    TextPosition    len;
    GC              normalgc  = text->gc;
    GC              selectgc  = text->invgc;
    TextPosition    offset;
    int             x;
    int             r;
    UnitPosition    wrap_bpos;
    UnitPosition    sbeg_bpos;
    UnitPosition    send_bpos;
    WrapDrawFunc    draw_wrap_line = wrap_draw[rep];
    PreEditRec	    pre_edit_rec = {0,0,NULL,NULL};
    PreEditRec	    *pre_edit_ptr;

    OlTextMarginCallData margin_call_data;

#ifdef DEBUG_TEXTDISP
    fprintf(stderr,"pageht = %d lineht = %d lineVisible = %d\n",
	    PAGEHT(ctx), text-> lineHeight, text-> linesVisible);
#endif

    if (!text-> updateState || text-> linesVisible < 1 || !XtIsRealized((Widget)ctx)) 
	return;
    if (rect-> width == 0 || rect-> height == 0) {
	margin_call_data.hint = OL_MARGIN_CALCULATED;
	rect = CalculateRectangle(ctx);
    } else {
	margin_call_data.hint = OL_MARGIN_EXPOSED;
	CalculateDisplayTable(ctx);
    }
    margin_call_data.rect = rect;

#ifdef DEBUG_TEXTDISP
    (void) fprintf(stderr,"rect: %d,%d %dx%d fontht = %d tmargin = %d\n", 
		   rect-> x, rect-> y, rect-> width, rect-> height, fontht, PAGE_T_MARGIN(ctx));
#endif

    start = rect-> y - PAGE_T_MARGIN(ctx);
    end   = start + rect-> height;
    start = MAX(0, start) / fontht;
    start = MIN(start, (text-> linesVisible - 1));
    end   = MAX(0, end) / fontht;
    end   = MIN(end, (text-> linesVisible - 1));

#ifdef DEBUG_TEXTDISP
	(void) fprintf(stderr, "new: Start = %d end = %d vis = %d\n",
    		start, end, text->linesVisible);
#endif

    _TurnTextCursorOff(ctx);
    XSetClipRectangles(XtDisplay(ctx), normalgc, 0, 0, rect, 1, Unsorted);
    XSetClipRectangles(XtDisplay(ctx), selectgc, 0, 0, rect, 1, Unsorted);
    DT = text-> DT;

    /* JIT: Compute cursor position, only if you need to */
    if (text->cursorLocation.line >= wrapTable->cur_start &&
	text->cursorLocation.line <= wrapTable->cur_end)
    {
	insline = _WrapLocationOfLocation(wrapTable, text-> cursorLocation);
	(void) _MoveToWrapLocation(wrapTable, DT[0].wraploc, insline, &r);
    }

    if (DT[start].p != NULL) {
	wrappos = _PositionOfWrapLocation(textBuffer, wrapTable, DT[start].wraploc);
	sensitive = XtIsSensitive((Widget)ctx) ? True : False;

	if (rep == OL_SB_STR_REP) {
	    wrap_bpos = wrappos;
	    sbeg_bpos = text->selectStart;
	    send_bpos = text->selectEnd;
	} else {
	    OlTextBufferPtr mltx = (OlTextBufferPtr)textBuffer;
	
	    wrap_bpos = OlUnitPositionOfTextPosition(mltx, wrappos);
	    sbeg_bpos = (text->selectStart > 0)?
		OlUnitPositionOfTextPosition(mltx, text->selectStart):text->selectStart;
	    send_bpos = (text->selectEnd > 0)?
		OlUnitPositionOfTextPosition(mltx, text->selectEnd): text->selectEnd;
	}

	if(rep != OL_SB_STR_REP &&
	   text->pre_edit_end > text->pre_edit_start) {
	    OlTextBufferPtr mltx = (OlTextBufferPtr)textBuffer;

	    pre_edit_rec.start = 
		OlUnitPositionOfTextPosition(mltx,text->pre_edit_start);
	    pre_edit_rec.end = 
		OlUnitPositionOfTextPosition(mltx,text->pre_edit_end);
	pre_edit_rec.feedbackGCs = text->feedbackGCs;
    }

    for (i = start, y = (i * fontht) + ascent + PAGE_T_MARGIN(ctx);
	 i <= end; 
	 i++, y += fontht)
    {
	len = DT[i].used;
	p   = DT[i].p;

	if(p == NULL) {
	    XClearArea(XtDisplay(ctx), XtWindow(ctx), PAGE_L_MARGIN(ctx), y - ascent, 
		       PAGEWID(ctx), PAGE_B_MARGIN(ctx) - (y - ascent), False);
	    break;
	}
	_StringOffsetAndPosition(p, 0, len, fs, rep, tabs, -text-> xOffset, &x, &offset);
	
	if(rep != OL_SB_STR_REP &&
			text->pre_edit_end > text->pre_edit_start){ 
		pre_edit_rec.pre_edit_feedback = DT[i].pre_edit_feedback;
		pre_edit_ptr = &pre_edit_rec;
	} else 
		pre_edit_ptr = NULL;

	draw_wrap_line(PAGE_L_MARGIN(ctx) + x, PAGE_L_MARGIN(ctx), PAGE_R_MARGIN(ctx),
		       p, offset, len, 
		       fs, tabs, text->wrapMode, XtDisplay(ctx), XtWindow(ctx), 
		       normalgc, selectgc, y, ascent, fontht,
		       wrap_bpos + offset, sbeg_bpos, send_bpos,
		       ctx->core.background_pixel, ctx->primitive.font_color,
		       text->xOffset,sensitive, pre_edit_ptr);
	wrap_bpos += len;
	}
    } else {
	int y = (start * fontht) + ascent + PAGE_T_MARGIN(ctx);
	XClearArea(XtDisplay(ctx), XtWindow(ctx), PAGE_L_MARGIN(ctx), y - ascent, 
		   PAGEWID(ctx), PAGE_B_MARGIN(ctx) - (y - ascent), False);
	
    }
    XSetClipMask(XtDisplay(ctx), normalgc, None);
    XSetClipMask(XtDisplay(ctx), selectgc, None);
    XtCallCallbacks((Widget)ctx, XtNmargin, &margin_call_data);
    /* margin was here!! */
    
    /* JIT: Draw text cursor only if you have to */
    if (text->cursorLocation.line >= wrapTable->cur_start &&
	text->cursorLocation.line <= wrapTable->cur_end)
    {
	(void) _DrawTextCursor(ctx, r, insline, 
		text-> cursorLocation.offset, ACTIVE_CARET(ctx) );
    }
} /* end of _DisplayText */

/*
 * _ChangeTextCursor
 *
 */
extern void
_ChangeTextCursor(TextEditWidget ctx)
{
TextEditPart * text      = &ctx-> textedit;

#if (1)
TextCursor *   tcp  = text-> CursorP;
if (text-> cursor_state == OlCursorOn || text-> cursor_state == OlCursorBlinkOff)
   {
   if (ACTIVE_CARET(ctx))
      {
#ifdef DEBUG_CURSOR
fprintf(stderr,"%s %d %s %d %d\n", __FILE__, __LINE__, "out to in", text-> cursor_state, text->editMode);
#endif
      if (text-> cursor_state == OlCursorOn)
      XCopyArea(XtDisplay(ctx), text-> CursorOut, XtWindow(ctx), text-> insgc,
         0, 0, tcp-> width, tcp-> height, 
         text-> cursor_x - tcp-> xoffset, text-> cursor_y - tcp-> baseline);
      XCopyArea(XtDisplay(ctx), text-> CursorIn, XtWindow(ctx), text-> insgc,
         0, 0, tcp-> width, tcp-> height, 
         text-> cursor_x - tcp-> xoffset, text-> cursor_y - tcp-> baseline);
      }
   else
      {
#ifdef DEBUG_CURSOR
fprintf(stderr,"%s %d %s %d\n", __FILE__, __LINE__, "in to out", text-> cursor_state);
#endif
      if (text-> cursor_state == OlCursorOn)
      XCopyArea(XtDisplay(ctx), text-> CursorIn, XtWindow(ctx), text-> insgc,
         0, 0, tcp-> width, tcp-> height, 
         text-> cursor_x - tcp-> xoffset, text-> cursor_y - tcp-> baseline);
      XCopyArea(XtDisplay(ctx), text-> CursorOut, XtWindow(ctx), text-> insgc,
         0, 0, tcp-> width, tcp-> height, 
         text-> cursor_x - tcp-> xoffset, text-> cursor_y - tcp-> baseline);
      }
   text-> cursor_state = OlCursorOn;
   }
#else
WrapTable *    wrapTable = text-> wrapTable;
TextLocation   currentIP;
TextLocation   currentDP;
TextLocation   wrapline;
TextLocation   insline;
int            r;

currentIP = text-> cursorLocation;
currentDP = text-> displayLocation;

wrapline = _WrapLocationOfLocation(wrapTable, currentDP);
insline  = _WrapLocationOfLocation(wrapTable, currentIP);
(void) _MoveToWrapLocation(wrapTable, wrapline, insline, &r);

(void) _DrawTextCursor(ctx, r, insline, currentIP.offset, ACTIVE_CARET(ctx));
(void) _DrawTextCursor(ctx, r, insline, currentIP.offset, !ACTIVE_CARET(ctx));
#endif

if (text-> blink_timer != NULL)
   {
   XtRemoveTimeOut(text-> blink_timer);
   text-> blink_timer = NULL;
   }
if (text-> blinkRate != 0L)
   if (ACTIVE_CARET(ctx) /* && text-> cursor_state == OlCursorOn */)
      text-> blink_timer = XtAppAddTimeOut(
				XtWidgetToApplicationContext((Widget)ctx),
				text-> blinkRate, _BlinkCursor, ctx);

} /* end of _ChangeTextCursor */
/*
 * _BlinkCursor
 *
 */

static void
_BlinkCursor(XtPointer client_data, XtIntervalId *id)
{
TextEditWidget ctx     = (TextEditWidget)client_data;
TextEditPart * text    = &ctx-> textedit;
TextCursor *   tcp     = text-> CursorP;
int x                  = text-> cursor_x;
int y                  = text-> cursor_y;
Display *      display = XtDisplay(ctx);
Window         window  = XtWindow(ctx);
long           rate    = text-> blinkRate;
XEvent         expose_event;
int	       statechange=0;

if (text->cursor_state == OlCursorOff) {
   statechange = TRUE;
   text->cursor_state = OlCursorBlinkOff;
}

if (XtIsRealized((Widget)ctx)) {

if (XCheckWindowEvent(display, window, ExposureMask, &expose_event))
   XPutBackEvent(display, &expose_event);
else
   {
   if (text-> cursor_state == OlCursorOn &&
       PAGE_L_MARGIN(ctx) <= x && x <= PAGE_R_MARGIN(ctx) &&
       text-> shouldBlink == TRUE)
      {
#ifdef DEBUG_CURSOR
fprintf(stderr,"%s %d %s\n", __FILE__, __LINE__, "blink off");
#endif
      XCopyArea(display, text-> CursorIn, window, text-> insgc, 
         0, 0, tcp-> width, tcp-> height, 
         x - tcp-> xoffset, y - tcp-> baseline);
      text-> cursor_state = OlCursorBlinkOff;
      XFlush(display);
      }
   else
      {
      if (text-> cursor_state == OlCursorBlinkOff &&
          PAGE_L_MARGIN(ctx) <= x && x <= PAGE_R_MARGIN(ctx) &&
          text-> shouldBlink == TRUE)
         {
#ifdef DEBUG_CURSOR
fprintf(stderr,"%s %d %s\n", __FILE__, __LINE__, "blink on");
#endif
         XCopyArea(display, text-> CursorIn, window, text-> insgc, 
            0, 0, tcp-> width, tcp-> height, 
            x - tcp-> xoffset, y - tcp-> baseline);
         text-> cursor_state = OlCursorOn;
         XFlush(display);
         rate = rate / 2;
         }
      else
	if (statechange)
	  {
	   statechange = FALSE;
	   text->cursor_state = OlCursorOff;
	  }
      }
   }
}

text-> blink_timer = XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)ctx),
				     rate, _BlinkCursor, (XtPointer)ctx);

} /* end of _BlinkCursor */
/*
 * _DrawTextCursor
 *
 */

extern int
_DrawTextCursor(TextEditWidget ctx, int r, TextLocation line, TextPosition offset, int flag)
{
    TextEditPart * text          = &(ctx-> textedit);
    OlFont         fs            = ctx->primitive.font;
    WrapTable *    wrapTable     = text-> wrapTable;
    TextBuffer *   textBuffer    = text-> textBuffer;
    int            fontht        = text-> lineHeight;
    TabTable       tabs          = text-> tabs;
    TextCursor *   tcp           = text-> CursorP;
    int            x;
    int            y;
    char *         p;
    int            retval        = 0;
    OlStrRep       rep           = text->text_format;
    int 	   ascent = ASCENT(ctx);
    XPoint	   spot = {0,0};
    OlInputContextID   ic_id = ctx->textedit.ic_id; 
    Arg		   args[2];

    if (r >= 0 && r < LINES_VISIBLE(ctx)) {
	p = (rep == OL_SB_STR_REP)?
	    GetTextBufferLocation(textBuffer, line.line, NULL):
            OlGetTextBufferStringAtLine((OlTextBufferPtr)textBuffer, line.line, NULL);

	y = PAGE_T_MARGIN(ctx) + ascent + r * fontht;
	if(wrapTable->text_format != OL_SB_STR_REP) {
    		TextLocation   temp;

		if(offset > 0) {
		temp.line = line.line;
		temp.offset = offset - 1;
		offset = OlUnitOffsetOfLocation((OlTextBufferPtr)
						textBuffer,&temp);
		} else
			offset -= 1; 
	} else
		offset -= 1;

	x = (offset >= 0 ? _StringWidth(0, p, 
			_WrapLineUnitOffset(textBuffer, wrapTable, line),
		      			offset, fs, rep, tabs) : 
					(UnitPosition) 0 )
	    			+ PAGE_L_MARGIN(ctx) + text-> xOffset;

	if (text-> updateState      && 
	    text-> selectMode != 6  &&
	    text-> selectMode != 8  &&
	    text-> selectMode != 9  &&
	    text-> selectMode != 65 &&
	    text-> selectMode != 85 &&
	    text-> selectMode != 95 &&
	    text-> cursor_visible   &&
	    (x >= (int)PAGE_L_MARGIN(ctx)) && (x <= PAGE_R_MARGIN(ctx)))
	 {
#ifdef DEBUG_CURSOR
	     fprintf(stderr,"%s %d %s\n", __FILE__, __LINE__, "draw it");
#endif
	     if (flag)
		 XCopyArea(XtDisplay(ctx), text-> CursorIn, XtWindow(ctx), 
			   text-> insgc, 0, 0, tcp-> width, tcp-> height, 
			   x - tcp-> xoffset, y - tcp-> baseline);
	     else
		 XCopyArea(XtDisplay(ctx), text-> CursorOut, XtWindow(ctx), 
			   text-> insgc, 0, 0, tcp-> width, tcp-> height, 
			   x - tcp-> xoffset, y - tcp-> baseline);
	     text-> cursor_state = OlCursorOn;
	     if(ic_id != (OlInputContextID)NULL && 
			ctx->textedit.pre_edit_style == OL_OVER_THE_SPOT) { 
			Arg args[2];
  			Cardinal i =0;

	     		spot.x = x;
	     		spot.y = y;
			XtSetArg(args[i],(String)XNSpotLocation,
							(XtArgVal)&spot); i++;
			OlSetValuesIC(ic_id, (Boolean)True,args,i);
	     } else if(text->pre_edit == TRUE && 
				text->pre_edit_start ==
				text->pre_edit_end)
				text->pre_edit_start =
				text->pre_edit_end   = text->cursorPosition;

	 } else
	     text-> cursor_state = OlCursorOff;
	text-> cursor_x = x;
	text-> cursor_y = y;
	retval = 1;
    } else
	text-> cursor_state = OlCursorOff;

#if (0)
    if (text-> blink_timer != NULL) {
	XtRemoveTimeOut(text-> blink_timer);
	text-> blink_timer = NULL;
    }
    if (text-> blinkRate != 0L)
	if (flag && text-> cursor_state == OlCursorOn)
	    text-> blink_timer = XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)ctx),
						 text-> blinkRate, _BlinkCursor, ctx);
#endif
    return (retval);
} /* end of _DrawTextCursor */

/*
 * _TurnTextCursorOff
 *
 */
extern void
_TurnTextCursorOff(TextEditWidget ctx)
{
TextEditPart * text = &ctx-> textedit;
TextCursor *   tcp  = text-> CursorP;

if (text-> updateState && text-> cursor_state == OlCursorOn)
   {
#ifdef DEBUG_CURSOR
fprintf(stderr,"%s %d %s %d %d\n", __FILE__, __LINE__, "turn off",text-> cursor_state, text->editMode);
#endif
   if (ACTIVE_CARET(ctx))
      XCopyArea(XtDisplay(ctx), text-> CursorIn, XtWindow(ctx), text-> insgc,
         0, 0, tcp-> width, tcp-> height, 
         text-> cursor_x - tcp-> xoffset, text-> cursor_y - tcp-> baseline);
   else
      XCopyArea(XtDisplay(ctx), text-> CursorOut, XtWindow(ctx), text-> insgc,
         0, 0, tcp-> width, tcp-> height, 
         text-> cursor_x - tcp-> xoffset, text-> cursor_y - tcp-> baseline);
   text-> cursor_state = OlCursorOff;
   }

#if (0)
if (text-> blink_timer != NULL)
   {
   XtRemoveTimeOut(text-> blink_timer);
   text-> blink_timer = NULL;
   }
#endif

} /* end of _TurnTextCursorOff */
/*
 * _CreateTextCursors
 *
 */

extern void
_CreateTextCursors(TextEditWidget ctx)
{
TextEditPart * text   = &ctx-> textedit;
int            fontht = FONTHT(ctx);
TextCursor *   tcp;
register int   i;

static TextCursor TextCursors[] = {
	/* This is not used . */
    { 8,  
	  smallIn_width,  smallIn_height, smallIn_x_hot, smallIn_y_hot, 
	  (unsigned char *)smallIn_bits, (unsigned char *)smallOut_bits },
	/* This is for 10pt and 12pt */
    { 15,  
	  mediumIn_width,  mediumIn_height, mediumIn_x_hot, mediumIn_y_hot, 
	  (unsigned char *)mediumIn_bits, (unsigned char *)mediumOut_bits },
	/* This is for 14pt and 16pt . */
    { 18,
	  pt16CurIn_width,  pt16CurIn_height, pt16CurIn_x_hot, pt16CurIn_y_hot,
          (unsigned char *)pt16CurIn_bits, (unsigned char *)pt16CurOut_bits },
        /* This is for 19pt . */
    { 20, 
	  largeIn_width,  largeIn_height, largeIn_x_hot, largeIn_y_hot, 
	  (unsigned char *)largeIn_bits, (unsigned char *)largeOut_bits },
	/* This is for 20pt . */
    {  22,
	   hugeIn_width,  hugeIn_height, hugeIn_x_hot, hugeIn_y_hot, 
	   (unsigned char *)hugeIn_bits, (unsigned char *)hugeOut_bits },
	/* This is for 24pt . */
    {  0,
           biggestCurIn_width, biggestCurIn_height, biggestCurIn_x_hot,
           biggestCurIn_y_hot, (unsigned char *)biggestCurIn_bits,
           (unsigned char *)biggestCurOut_bits },
};

if (text-> CursorIn != (Pixmap)0)
   {
   XFreePixmap(XtDisplay(ctx), text-> CursorIn);
   XFreePixmap(XtDisplay(ctx), text-> CursorOut);
   }

for (i = 0; TextCursors[i].limit != 0; i++)
   if (fontht <= TextCursors[i].limit)
      break;

tcp = text-> CursorP = &TextCursors[i];

text-> CursorIn  = XCreatePixmapFromBitmapData(XtDisplay(ctx),
                 RootWindowOfScreen(XtScreen(ctx)),
                 (char *)tcp-> inbits, tcp-> width, tcp-> height, 
                 ctx->primitive.input_focus_color ^ ctx-> core.background_pixel, (Pixel)0, 
                 ctx-> core.depth);
text-> CursorOut = XCreatePixmapFromBitmapData(XtDisplay(ctx),
                 RootWindowOfScreen(XtScreen(ctx)),
                 (char *)tcp-> outbits, tcp-> width, tcp-> height, 
                 ctx->primitive.input_focus_color ^ ctx-> core.background_pixel, (Pixel)0, 
                 ctx-> core.depth);

} /* end of _CreateTextCursors */

static UnitPosition
sb_strnmatch(OlStr s1, OlStr s2, int nunits, int *chars_same)
{
    register char *  ss1  = (char *) s1;
    register char *  ss2  = (char *) s2;
    char *           save = ss1;

    *chars_same = -1;
    if(ss1 == NULL || ss2 == NULL) 
	return -1;		/* defend against NULL pointers */

    while (*ss1 == *ss2 && nunits) {
	ss1++;
	ss2++;
	nunits--;
	(*chars_same)++;
    }

    if (ss1 == save)
	return(-1);
     else 
	return(ss1 -save  -1);
    
}

static UnitPosition
mb_strnmatch(OlStr s1, OlStr s2, int nunits, int *chars_same)
{
    register char *  ms1 = (char *)s1;
    register char *  ms2 = (char *)s2;
    register int     cl1;
    register int     cl2;
    char *           save = ms1;
    char *prev = NULL;
    wchar_t w1;
    wchar_t w2;

    *chars_same = -1;
    if(ms1 == NULL || ms2 == NULL) 
	return -1;		/* guard against NULL pointers */

    for(*chars_same = -1,cl1 = mbtowc(&w1, ms1, MB_CUR_MAX),
	cl2 = mbtowc(&w2, ms2, MB_CUR_MAX); 
	cl1>0 && cl2 > 0 &&  w1 == w2 && nunits > 0; nunits -= cl2, 
					prev = ms1, ms1 += cl1, ms2 += cl2,
					(*chars_same)++,
    					cl1 = mbtowc(&w1, ms1, MB_CUR_MAX),
					cl2 = mbtowc(&w2, ms2, MB_CUR_MAX) )
			;

     if (ms1 == save) 
	if(cl1 == 0 && cl2 == 0 && nunits > 0) { 
		(*chars_same)++;
		return 0;
	} else 
		return -1;
     else if(cl1 == 0 && cl2 == 0) {
	(*chars_same)++;
	return(prev - save +1);
     } else
	return(prev - save);
    
}

static UnitPosition
wc_strnmatch(OlStr s1, OlStr s2 , int nunits, int *chars_same)
{
    wchar_t * ws1 = (wchar_t *)s1;
    wchar_t * ws2 = (wchar_t *)s2;
    wchar_t * save = ws1;

    *chars_same = -1;
    if(ws1 == (wchar_t *)NULL || ws2 == (wchar_t *)NULL) 
	return -1;		/* defend against NULL pointers */

    while (*ws1 == *ws2 && nunits) {
	ws1++;
	ws2++;
	nunits --;
	(*chars_same)++;
    }

     if (ws1 == save) 
	return(-1);
     else 
	return(ws1 - save - 1);
}

void _MakeTextCursorVisible(TextEditWidget ctx)
{
TextEditPart * text = &ctx-> textedit;
TextCursor *   tcp  = text-> CursorP;
OlInputContextID   ic_id = ctx->textedit.ic_id; 
 
	if(text->cursor_state != OlCursorOff)
                return;
 
	if(ACTIVE_CARET(ctx))
        	XCopyArea(XtDisplay(ctx), text-> CursorIn,
                                XtWindow(ctx), text-> insgc,
                                0, 0, tcp-> width, tcp-> height,
                                text-> cursor_x - tcp-> xoffset,
                                text-> cursor_y - tcp-> baseline);
	else
        	XCopyArea(XtDisplay(ctx), text-> CursorOut,
                                XtWindow(ctx), text-> insgc,
                                0, 0, tcp-> width, tcp-> height,
                                text-> cursor_x - tcp-> xoffset,
                                text-> cursor_y - tcp-> baseline);

        if (text-> blink_timer != NULL) {
                XtRemoveTimeOut(text-> blink_timer);
                text-> blink_timer = NULL;
        }

	if(ACTIVE_CARET(ctx))
        	if (text-> blinkRate != 0L)
               	 text-> blink_timer = XtAppAddTimeOut(
                                XtWidgetToApplicationContext((Widget)ctx),
                                text-> blinkRate, _BlinkCursor, ctx);

        text->cursor_visible = True;
        text->cursor_state = OlCursorOn;

	if(ic_id != (OlInputContextID)NULL && 
		ctx->textedit.pre_edit_style == OL_OVER_THE_SPOT) { 
		Arg args[2];
  		Cardinal i =0;
		XPoint spot;

     		spot.x = text-> cursor_x;
     		spot.y = text-> cursor_y;
		XtSetArg(args[i],(String)XNSpotLocation,
						(XtArgVal)&spot); i++;
		OlSetValuesIC(ic_id, (Boolean)True,args,i);
	}
}

void _TurnCursorBlinkOn(TextEditWidget ctx)
{
TextEditPart * text = &ctx-> textedit;
TextCursor *   tcp  = text-> CursorP;

        if (text-> blink_timer != NULL) {
                XtRemoveTimeOut(text-> blink_timer);
                text-> blink_timer = NULL;
        }

	if(ACTIVE_CARET(ctx))
        	if (text-> blinkRate != 0L)
               	 text-> blink_timer = XtAppAddTimeOut(
                                XtWidgetToApplicationContext((Widget)ctx),
                                text-> blinkRate, _BlinkCursor, ctx);
}
