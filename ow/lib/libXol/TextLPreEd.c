#pragma ident	"@(#)TextLPreEd.c	1.9	97/03/26 lib/libXol SMI"	/* OLIT	*/
/*
 *        Copyright (C) 1986,1991  Sun Microsystems, Inc
 *                    All rights reserved.
 *          Notice of copyright on this source code 
 *          product does not indicate publication. 
 * 
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by 
 * the U.S. Government is subject to restrictions as set forth 
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988) 
 * and FAR 52.227-19 (c) (June 1987).
 *
 *    Sun Microsystems, Inc., 2550 Garcia Avenue,
 *    Mountain View, California 94043.
 *
 */

#include 	<locale.h>
#include	<stdlib.h>

#include	<Xol/TextLBuffI.h>
#include 	<Xol/TextLineP.h>
#include	<Xol/TextLineI.h>

#define SIZE 10

Private void ReplacePreedStr(TextLineWidget w,int start, int length,OlStr string);
Private void ReplaceFeedback(TextLineWidget w, int start, int length, 
				char * string,int str_length);

/*****************************************************************************
		_OlTLPreeditStartCallbackFunc
*****************************************************************************/
WidgetInternal int 
_OlTLPreeditStartCallbackFunc(XIC ic, XPointer client_data, XPointer call_data)
{
	TextLineWidget w = (TextLineWidget)client_data;
	TextLinePart *tlp = &w->textLine;
	OlStrRep format = w->primitive.text_format;

	/* Get rid of any selections here ... */

	tlp->preed_on = True;
	tlp->preed_start = tlp->caret_pos;

#define WC_NULL_LEN 4
#define NULL_LEN 1

	if (tlp->preed_buffer == (TLBuffer *)NULL) {	/* First time */

		tlp->preed_buffer = _OlTLAllocateBuffer(SIZE);
		_OlTLInsertBytes(tlp->preed_buffer, _OLStrEmptyString(format),  0, 
				 format == OL_WC_STR_REP ? WC_NULL_LEN: NULL_LEN
				);

		if (format == OL_MB_STR_REP) {
			tlp->preed_pos_table = _OlTLAllocateBuffer(SIZE);
			*((int *)(tlp->preed_pos_table->p)) = 0;
			tlp->preed_pos_table->used = sizeof(int);
		}
		else
			tlp->preed_pos_table = NULL;

		tlp->feedback_table = _OlTLAllocateBuffer(SIZE);

		format = OL_WC_STR_REP;	/*  wchar_t is long ! */
		_OlTLInsertBytes(tlp->feedback_table, _OLStrEmptyString(format), 0, 
				 sizeof(unsigned long)
				);
	}
	return -1;
#undef NULL_LEN
#undef WC_NULL_LEN
}

/*****************************************************************************
		_OlTLPreeditEndCallbackFunc
*****************************************************************************/
WidgetInternal void 
_OlTLPreeditEndCallbackFunc(XIC ic, XtPointer client_data, XtPointer call_data)
{
	TextLineWidget w = (TextLineWidget)client_data;
	TextLinePart *tlp = &w->textLine;  

	tlp->preed_on = False;
	tlp->preed_start = -1;
	tlp->num_preed_chars = 0;
	tlp->caret_pos = tlp->cursor_position;
}

/*****************************************************************************
		_OlTLPreeditDrawCallbackFunc
*****************************************************************************/
WidgetInternal void
_OlTLPreeditDrawCallbackFunc(XIC ic, XPointer client_data, 
			XIMPreeditDrawCallbackStruct *pDraw)
{
	TextLineWidget w = (TextLineWidget)client_data;
	TextLinePart *tlp = &w->textLine;
	OlStr buffer = (OlStr)NULL;
	XIMFeedback * fdbkBuffer = (XIMFeedback *)NULL;
	int length_fdbk_buffer = 0;
	OlStrRep pFormat , tFormat;
	void* toFree[2] = { NULL, NULL };
	unsigned long what;
	int start;

	tFormat = w->primitive.text_format;
	if (pDraw->text != (XIMText *)NULL) {
		pFormat = pDraw->text->encoding_is_wchar ? OL_WC_STR_REP:
							   OL_MB_STR_REP;
		buffer = pFormat == OL_MB_STR_REP ? 
					  (OlStr)pDraw->text->string.multi_byte:
					  (OlStr)pDraw->text->string.wide_char;

		if (buffer != NULL && pFormat != tFormat) {
		/* Stuff from brain-dead IM is not in our format. So convert ..*/
			 switch(pFormat) {
			   case OL_MB_STR_REP:
				buffer = toFree[0] = (OlStr)
				  XtCalloc(pDraw->text->length+1, sizeof(wchar_t));
				(void)mbstowcs((wchar_t *)buffer,
					pDraw->text->string.multi_byte,
					pDraw->text->length);
				break;
			   default:
				buffer = toFree[0] = (OlStr)
				 XtCalloc((pDraw->text->length+1) * MB_CUR_MAX,
						sizeof(char));
				 (void)wcstombs((char *)buffer,
					pDraw->text->string.wide_char,
					(pDraw->text->length) * MB_CUR_MAX);
				break;
			 }
		}

		if (pDraw->text->feedback == NULL) 
			fdbkBuffer = toFree[1] = 
				(XIMFeedback *)XtCalloc(pDraw->text->length,
					sizeof (XIMFeedback));
		else
			fdbkBuffer = pDraw->text->feedback;
		length_fdbk_buffer = pDraw->text->length;
	}

	ReplacePreedStr(w, pDraw->chg_first, pDraw->chg_length, buffer);
	ReplaceFeedback(w, pDraw->chg_first, pDraw->chg_length, 
				(char *)fdbkBuffer,length_fdbk_buffer);

/* pDraw->caret is the position AFTER which the cursor should move ..ref X11R5spec */
	tlp->preed_caret = pDraw->caret + tlp->preed_start;	

	tlp->caret_pos = tlp->preed_caret + tlp->preed_start;
	tlp->num_preed_chars += ((pDraw->text != NULL ? pDraw->text->length : 0) -
				 pDraw->chg_length);

	if ((what = _OlTLSetupArrowsAfterTextChange(w)) & TLDrawText) 
		start = tlp->char_offset + _OLTLNumArrowChars(tlp);
	else
		start = pDraw->chg_first + tlp->preed_start;
	_OlTLDrawWidget(w, start, TLEndOfDisplay, what | TLDrawText | TLMoveCaret);

	if (toFree[0]) XtFree(toFree[0]);
	if (toFree[1]) XtFree(toFree[1]);
} 

/*****************************************************************************
		ReplacePreedStr

  NOTE:	Assumption that "string" as returned thru the DrawCallback
  is NULL terminated. If not, we should think about passing the length of
  string too into _OlTLInsertString .... 
*****************************************************************************/
Private void
ReplacePreedStr(TextLineWidget w, int start, int length, OlStr string)
{
	TextLinePart *tlp = &w->textLine;

	if (length)
		_OlTLDeleteString(tlp->preed_buffer, start, start+length, 
				  w->primitive.text_format, tlp->preed_pos_table);
	if (string != (OlStr)NULL)
		_OlTLInsertString(tlp->preed_buffer, string, start, 
				w->primitive.text_format, tlp->preed_pos_table);
}

/*****************************************************************************
		ReplaceFeedback
*****************************************************************************/
Private void
ReplaceFeedback(TextLineWidget w, int start, int length, char * string,int str_length)
{
	TextLinePart *tlp = &w->textLine;
	int end = start + length;

/* NOTE that "string" is NOT null-terminated, and "string" is an array 
 * of XIMFeedbacks -hence cannot & no-need to use _Ol(*)String routines ..
 */
	if (length)
		_OlTLDeleteBytes(tlp->feedback_table,
			start* sizeof(XIMFeedback),end * sizeof(XIMFeedback));
	if (string != (char *)NULL)
		_OlTLInsertBytes(tlp->feedback_table, string, 
			start * sizeof(XIMFeedback), str_length * sizeof(XIMFeedback));
}

/*****************************************************************************
		_OlTLPreeditCaretCallbackFunc
*****************************************************************************/
WidgetInternal void 
_OlTLPreeditCaretCallbackFunc(XIC ic,XPointer client_data,
			XIMPreeditCaretCallbackStruct *call_data)
{
	TextLineWidget w = (TextLineWidget)client_data;
	TextLinePart *tlp = &w->textLine;
	unsigned long what;
	int start = tlp->preed_start;
	int end = tlp->preed_start + tlp->num_preed_chars - 1;
	int pos = tlp->caret_pos;

	switch(call_data->direction) {
		case XIMForwardChar:
			pos = (pos == end ? pos : pos++);
			break;
		case XIMBackwardChar:
			pos = (pos == start ? pos : pos--);
			break;
		case XIMLineStart:
			pos = start;
			break;
		case XIMLineEnd:
			pos = end;
			break;
		case XIMAbsolutePosition:
			pos = (start + call_data->position <= end ?
				start + call_data->position : pos);
			break;
		case XIMDontChange:
			break;
		default:
			break;
	}

	tlp->preed_caret = pos - tlp->preed_start;
	tlp->caret_pos = pos;

	what = _OlTLSetupArrowsAfterCursorChange(w);
	what |= TLMoveCaret;
	_OlTLDrawWidget(w, _OLTLStartOfDisplay(tlp), TLEndOfDisplay, what);
}
