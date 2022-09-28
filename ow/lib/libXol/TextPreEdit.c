#pragma ident	"@(#)TextPreEdit.c	302.17	97/03/26 lib/libXol SMI" /* OLIT	*/

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

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#include <Xol/OpenLookP.h>
#include <Xol/TextEditP.h>
#include <Xol/TextEPos.h>
#include <Xol/TextDisp.h>
#include <Xol/buffutil.h>
#include <Xol/OlStrMthdsI.h>
#include <Xol/Oltextbuff.h>
#include <Xol/OlI18nP.h>
#include <Xol/OlCursors.h>


static const size_t peb_alloc = 64;
static const size_t num_feedbacks = 2;

extern int PreeditStartCallbackFunc(XIC ic,
			XPointer client_data,
			XPointer call_data)
{
	TextEditWidget ctx = (TextEditWidget)client_data;
	int i,j;

	ctx->textedit.pre_edit = True;
	ctx->textedit.pre_edit_start = 
	ctx->textedit.pre_edit_end  = ctx->textedit.cursorPosition;
		/* alloc the feedback buffer */
	if(ctx->textedit.pre_edit_feedback == (XIMFeedbackBuffer *)NULL)
		ctx->textedit.pre_edit_feedback = (XIMFeedbackBuffer *)
			AllocateBuffer(sizeof(XIMFeedback),peb_alloc);

		/* alloc the feedback GCs */
	if(ctx->textedit.feedbackGCs == (GC *)NULL) {
		ctx->textedit.feedbackGCs = (GC*)XtCalloc(num_feedbacks, sizeof (GC));
			/* fill in the GCs */
		ctx->textedit.feedbackGCs[0] = ctx->textedit.invgc; /* reverse*/
		ctx->textedit.feedbackGCs[1] = ctx->textedit.gc; /* text underline */

	}
			
	return -1;
}

void PreeditEndCallbackFunc(XIC ic, 
			XPointer client_data,
			XPointer call_data)
{
	TextEditWidget ctx = (TextEditWidget)client_data;
	
	ctx->textedit.pre_edit = False;
	ctx->textedit.pre_edit_feedback->used = 0;

}

void PreeditDrawCallbackFunc(XIC ic, 
			XPointer client_data,
			XIMPreeditDrawCallbackStruct *pdraw)
{
	TextEditWidget ctx = (TextEditWidget)client_data;
	OlTextBufferPtr text = (OlTextBufferPtr)ctx->textedit.textBuffer;
	OlStr buffer = (OlStr)NULL;
	int length;
	OlStrRep wtf = ctx->primitive.text_format;
	int i,j;
	TextLocation *start;
	TextLocation *end;
	static XIMFeedbackBuffer sbuffer;
	OlStrRep tf;
	Boolean	 free_buffer = FALSE;

	if(pdraw->text != NULL) {
	tf  = (pdraw->text->encoding_is_wchar ? OL_WC_STR_REP:
							OL_MB_STR_REP);
	if(tf == OL_MB_STR_REP)
		buffer = (OlStr)pdraw->text->string.multi_byte;
	else
		buffer = (OlStr)pdraw->text->string.wide_char;
	}
		 
	if(pdraw->text != (XIMText *)NULL && 
			buffer != (OlStr) NULL && tf != wtf)  

			/*
			 * IM can send back stuff to draw
			 * in any encoding, and we have no say.
			 * So if we are different, we convert.
			 */ 

		switch(tf) {
			/* check the format of stuff to draw */
			case OL_MB_STR_REP: 
				buffer = (OlStr)XtCalloc((pdraw->text->length+1),
								sizeof(wchar_t));	
				(void)mbstowcs((wchar_t *)buffer,
					pdraw->text->string.multi_byte,
					pdraw->text->length);
				free_buffer = TRUE;
				break;
			default: /* we are in multi byte */
				buffer = (OlStr)XtCalloc((pdraw->text->length+1)*
						MB_CUR_MAX, sizeof(char));
				(void)wcstombs((char *)buffer,
					pdraw->text->string.wide_char,
					(pdraw->text->length+1)*MB_CUR_MAX);
				free_buffer = TRUE;
				break;
		}

		/* feedback stuff */

		if(pdraw->chg_length > 0 &&
			pdraw->chg_length <= ctx->textedit.pre_edit_feedback->used) {

			if(buffer != (OlStr)NULL) {
				/* delete and insert feedback stuff */

				/* first delete */
				for(i=pdraw->chg_first + pdraw->chg_length, j= pdraw->chg_first ; 
						i < ctx->textedit.pre_edit_feedback->used; i++) 
					ctx->textedit.pre_edit_feedback->p[j] =
						ctx->textedit.pre_edit_feedback->p[i];

				ctx->textedit.pre_edit_feedback->used -= pdraw->chg_length;

				/* then insert */
				if(pdraw->text->feedback == NULL) 
					sbuffer.p = (XIMFeedback *)XtCalloc(
						(pdraw->text->length + 1),
						sizeof (XIMFeedback));
					else
						sbuffer.p = pdraw->text->feedback;
				sbuffer.size = sbuffer.used = pdraw->text->length+1;
				InsertIntoBuffer((Buffer *)ctx->textedit.pre_edit_feedback,
							(Buffer *)&sbuffer,pdraw->chg_first);
				
			} else {
				/* delete and change feedback stuff */

				/* first delete */
				for(i=pdraw->chg_first + pdraw->chg_length, j= pdraw->chg_first ; 
						i < ctx->textedit.pre_edit_feedback->used; i++) 
					ctx->textedit.pre_edit_feedback->p[j] =
						ctx->textedit.pre_edit_feedback->p[i];

				ctx->textedit.pre_edit_feedback->used -= pdraw->chg_length;

				/* then change */
				if(pdraw->text != NULL && pdraw->text->feedback != NULL) 
					memmove((XtPointer)
					&ctx->textedit.pre_edit_feedback->p[pdraw->chg_first],
						(XtPointer)pdraw->text->feedback,
						pdraw->text->length*sizeof(XIMFeedback));
			} 
		} else { 
			if(buffer  != (OlStr)NULL) {
				/* insert feedback stuff */
				if(pdraw->text->feedback == NULL) 
					sbuffer.p = (XIMFeedback *)XtCalloc(
						(pdraw->text->length + 1),
						sizeof (XIMFeedback));
					else
						sbuffer.p = pdraw->text->feedback;
				sbuffer.size = sbuffer.used = pdraw->text->length+1;
				InsertIntoBuffer((Buffer *)ctx->textedit.pre_edit_feedback,
							(Buffer *)&sbuffer,pdraw->chg_first);
			} else {
				/* change feed back stuff */
				if(pdraw->text != NULL && pdraw->text->feedback != NULL) 
				memmove((XtPointer)
					&ctx->textedit.pre_edit_feedback->p[pdraw->chg_first],
						(XtPointer)pdraw->text->feedback,
						pdraw->text->length*sizeof(XIMFeedback));
			}
		} 
			

	/* position the caret */
	ctx->textedit.pre_edit_caret = ctx->textedit.pre_edit_start + pdraw->caret;


	/* stuff to edit */
	if(pdraw->chg_length > 0 &&
			pdraw->chg_length <= 
			ctx->textedit.pre_edit_end - 
			ctx->textedit.pre_edit_start) { 

		if(ctx->textedit.selectEnd  !=
			ctx->textedit.selectStart) 
			_MoveSelection(ctx, 
				ctx->textedit.cursorPosition,0,0,0);

		if(buffer == (OlStr)NULL) { /* simple delete */
			ctx->textedit.pre_edit_end -=
						pdraw->chg_length;
			start = OlLocationOfPosition(text,
					ctx->textedit.pre_edit_start+
					(TextPosition)pdraw->chg_first,
					(TextLocation *)NULL);

			end = OlLocationOfPosition(text,
					ctx->textedit.pre_edit_start+
					(TextPosition)pdraw->chg_first+
					(TextPosition)pdraw->chg_length,
					(TextLocation *)NULL);

			OlReplaceBlockInTextBuffer(text,
					start,end,(OlStr)NULL,
					(TextUpdateFunction)UpdateDisplay, (XtPointer)ctx);
		} else {
			/* delete and insert */
			ctx->textedit.pre_edit_end += (pdraw->text->length -
						pdraw->chg_length);
					
			start = OlLocationOfPosition(text,
					ctx->textedit.pre_edit_start+
					(TextPosition)pdraw->chg_first,
					(TextLocation *)NULL);

			end = OlLocationOfPosition(text,
					ctx->textedit.pre_edit_start+
					(TextPosition)pdraw->chg_first+
					(TextPosition)pdraw->chg_length,
					(TextLocation *)NULL);

			OlReplaceBlockInTextBuffer(text,
					start,end,(OlStr)buffer,
					(TextUpdateFunction)UpdateDisplay, (XtPointer)ctx);
		}
	} else { 

		if(buffer != (OlStr)NULL) { /* simple insert */
			if(ctx->textedit.selectEnd != 
				ctx->textedit.selectStart) 
				_MoveSelection(ctx, 
					ctx->textedit.cursorPosition,0,0,0);
			ctx->textedit.pre_edit_end += pdraw->text->length;
					
			start = OlLocationOfPosition(text,
					ctx->textedit.pre_edit_start+
					(TextPosition)pdraw->chg_first,
					(TextLocation *)NULL);

			end = start;
			OlReplaceBlockInTextBuffer(text,
					start,end,(OlStr)buffer,
					(TextUpdateFunction)UpdateDisplay, (XtPointer)ctx);
			
		}
		
	}

	if(free_buffer && buffer != NULL)
		XtFree((char *)buffer);
}

void PreeditCaretCallbackFunc(XIC ic,
				XPointer client_data,
				XIMPreeditCaretCallbackStruct *call_data)
{
TextEditWidget ctx = (TextEditWidget) client_data;
TextPosition start = ctx->textedit.pre_edit_start;
TextPosition  end = ctx->textedit.pre_edit_end;   	
TextPosition cursor = ctx->textedit.cursorPosition;
OlTextBufferPtr text = (OlTextBufferPtr)ctx->textedit.textBuffer;
TextPosition position = cursor;

	switch(call_data->direction) {
		case XIMForwardChar:
			position += ( (cursor + 1) <= end ? 1: 0);
			break;
		case XIMBackwardChar:
			position -= ( (cursor - 1) >= start ? 1 : 0);
			break;
		case XIMCaretUp:
			{
				TextLocation templ; 
				TextPosition tempp;

				(void)OlLocationOfPosition(text,cursor,&templ);

				templ.line -= (templ.line - 1 >= 0 ? 1 : 0);
				
				tempp = OlPositionOfLocation(text,&templ);
				
				position = (tempp >= start ? tempp : position);
			}
			break;
		case XIMCaretDown:
			{
				TextLocation templ; 
				TextPosition tempp;
				TextLine last_line = OlLastTextBufferLine(text);

				(void)OlLocationOfPosition(text,cursor,&templ);

				templ.line += (templ.line + 1 <= last_line ? 1 : 0);
				
				tempp = OlPositionOfLocation(text,&templ);
				
				position = (tempp <= end ? tempp : position);
			}
			break;
		case XIMPreviousLine:
			{
				TextLocation templ; 
				TextPosition tempp;

				(void)OlLocationOfPosition(text,cursor,&templ);

				templ.line -= (templ.line - 1 >= 0 ? 1 : 0);
				templ.offset = 0;
				
				tempp = OlPositionOfLocation(text,&templ);
				
				position = (tempp >= start ? tempp : position);
			}
			break;
		case XIMNextLine:
			{
				TextLocation templ; 
				TextPosition tempp;
				TextLine last_line = OlLastTextBufferLine(text);

				(void)OlLocationOfPosition(text,cursor,&templ);

				templ.line += (templ.line + 1 <= last_line ? 1 : 0);
				templ.offset = 0;
				
				tempp = OlPositionOfLocation(text,&templ);
				
				position = (tempp <= end ? tempp : position);
			}
			break;
		case XIMLineStart:
			{
				TextLocation templ; 
				TextPosition tempp;

				(void)OlLocationOfPosition(text,cursor,&templ);

				templ.offset = 0;
				
				tempp = OlPositionOfLocation(text,&templ);
				
				position = (tempp >= start ? tempp : position);
			}
			break;
		case XIMLineEnd:
			{
				TextLocation templ; 
				TextPosition tempp;

				(void)OlLocationOfPosition(text,cursor,&templ);

				templ.offset = OlLastCharInTextBufferLine(text,templ.line);
				
				tempp = OlPositionOfLocation(text,&templ);
				
				position = (tempp <= end ? tempp : position);
			}
			break;
		case XIMAbsolutePosition:
			position =
					(start + call_data->position <= end ?
						start + call_data->position :
						cursor);
			break;
		default:
			break;
	}

	if(cursor != position) {
		ctx->textedit.pre_edit_caret = position;
		_MoveSelection(ctx,position,0,0,0);	
		if(call_data->style == XIMIsInvisible && 
				ctx->textedit.cursor_state == OlCursorOn)
			_TurnTextCursorOff(ctx);
		call_data->position = 
				ctx->textedit.pre_edit_caret - 
				ctx->textedit.pre_edit_start;
	}

}
