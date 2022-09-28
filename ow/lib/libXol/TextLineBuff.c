#pragma ident	"@(#)TextLineBuff.c	1.3	92/10/06 lib/libXol SMI"	/* OLIT	*/

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

#include 	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<widec.h>

#include 	<Xol/OlStrMthdsI.h>
#include	<Xol/TextLineP.h>
#include	<Xol/TextLineI.h>
#include	<Xol/TextLBuffI.h>


/* Private Functions */
Private void	GrowBuffer(TLBuffer *b, int increment);
Private void	InsertPos(PositionTable *posTable, int pos, OlStr string);
Private void	DeletePos(PositionTable *posTable, int start, int end);

#define INCR     10 /* This value should actually be different for the
		    * different format-types ... Ex :for MB = INCR*4 ?
		   */

/**************************************************************
		_OlTLAllocateBuffer

***************************************************************/
WidgetInternal TLBuffer *
_OlTLAllocateBuffer(int initial_size)
{
	TLBuffer *b = (TLBuffer *)XtMalloc(sizeof(TLBuffer));

	b->size = b->used = 0;
	b->p = NULL;

	if (initial_size != 0)
		GrowBuffer(b, initial_size);

	return (b);
}

/**************************************************************
		_OlTLFreeBuffer

***************************************************************/
WidgetInternal void
_OlTLFreeBuffer(TLBuffer *b)
{
	if (b->p)
		XtFree(b->p);
	XtFree((char *)b);
}

/**************************************************************
		GrowBuffer

***************************************************************/
static void
GrowBuffer(TLBuffer *b, int increment)
{
	b->size += increment;

	if (b->size == increment)
		b->p = (char *)XtMalloc((unsigned)(b->size));
	else
		b->p = (char *)XtRealloc(b->p, (unsigned)(b->size));
}


/**************************************************************
		_OlTLInsertBytes

***************************************************************/
WidgetInternal void
_OlTLInsertBytes(TLBuffer *b, char *buff, int pos, int num_bytes)
{

	int space_needed; 

#ifdef DEBUG
	if (num_bytes == 0 || pos < 0 || pos > b->used) {
		printf("_OlTLInsertBytes: num_bytes= %d pos= %d used= %d\n", 
			num_bytes, pos,b->used);
		abort();
	}
#endif

	space_needed = num_bytes - (b->size - b->used);

	if (space_needed > 0)
		GrowBuffer(b, space_needed + INCR);

	if (pos != b->used)
		memmove((void *)&b->p[pos + num_bytes],
				(void *)&b->p[pos],
				(b->used - pos));

	memmove((void *)&b->p[pos], (void *)buff, num_bytes);
	b->used += num_bytes;
}

/**************************************************************
		_OlTLDeleteBytes

 Deletes bytes from "start" till "end - 1" inclusive 
***************************************************************/
WidgetInternal void
_OlTLDeleteBytes(TLBuffer *b, int start, int end)
{

#ifdef DEBUG
	if (start >= end || start > b->used || end > b->used) {
		printf("_OlTLDeleteBytes: start= %d end= %d used= %d\n", 
			start, end,b->used);
		abort();
	}
#endif
	
	memmove((void *)&b->p[start], (void *)&b->p[end], 
			b->used - end);
	
	b->used -= (end - start);
}
	
/**************************************************************
		InsertPos

***************************************************************/
static void
InsertPos(PositionTable *posTable, int pos, OlStr string)
{
	int space_needed;
	int bytes_in_mbchar;
	int mbchars_in_line = 0;
	int bytes_left; 
	char *p; 
	int pos_b = pos * sizeof(int);
	int total_bytes = strlen((const char *)string) + 1;

	/* Inserting a MB char at cursor-pos "pos" will NOT
	 * affect the entry at posTable[pos]. It will change the
	 * entry at posTable[pos + 1] - ie it sets the start_pos
	 * for the next character. Hence we are NOT bothered about
	 * the value at posTable[pos+1] , since its going to get
	 * changed anyway. Thus we don't memmove it ....etc etc
	*/
	pos_b += sizeof(int);

#ifdef DEBUG
	if (pos < 0 || pos_b  > posTable->used) {
		printf("InsertPos: pos= %d pos_b= %d used= %d\n", 
			pos, pos_b,posTable->used);
		abort();
	}
#endif

	mbchars_in_line = _OLStrNumChars(OL_MB_STR_REP, string);
	if (mbchars_in_line == 0 || mbchars_in_line == -1)
		return;

	/* We will be inserting mbchars_in_line Positions into the
	 * position array .... So check for space ... */

	mbchars_in_line	*= sizeof(int); /* each mbchar's pos is stored
					as an int */
	space_needed = mbchars_in_line - 
			(posTable->size - posTable->used);
	if (space_needed > 0)
		GrowBuffer(posTable, space_needed + INCR);
	
	if (pos_b  != posTable->used)
		memmove(
		  (void *)&posTable->p[pos_b + mbchars_in_line],
		  (void *)&posTable->p[pos_b], 
		  (posTable->used - pos_b));
	

	for (p = string, bytes_left = total_bytes;
	    (bytes_in_mbchar = mblen((char *)p, (size_t)bytes_left)) != 0;
	    p += bytes_in_mbchar, bytes_left -= bytes_in_mbchar) {
		*((int *)(posTable->p) + pos+1) = 
			bytes_in_mbchar + *((int *)(posTable->p) + pos);
		pos++;
	}

	total_bytes -= 1;
	for(; pos_b < posTable->used; pos++,pos_b += sizeof(int))
		*((int *)(posTable->p) + pos +1) += total_bytes;

	posTable->used += mbchars_in_line;
}

/**************************************************************
		DeletePos

***************************************************************/
static void
DeletePos(PositionTable *posTable, int start, int end)
{
	int start_b = start * sizeof(int);
	int end_b = end * sizeof(int);
	int total_bytes = end_b - start_b;
	int i, decr = 0;

	/*
	 * Delete(start , end) deletes the chars between start &
	 * end-1 inclusive.
	 * Deleting a MB char at cursor-pos "pos" will NOT
	 * affect the entry at posTable[pos]. It will change the
	 * entry at posTable[pos + 1] 
	*/

	start_b += sizeof(int);
	end_b += sizeof(int);

#ifdef DEBUG
	if (start >= end || start_b > posTable->used || end_b > posTable->used) {
		printf("DeletePos: start= %d end= %d used= %d\n", 
			start, end,posTable->used);
		abort();
	}
#endif

	for(i = start; i < end; i++)
		decr += *((int *)(posTable->p) + i +1) -
			*((int *)(posTable->p) + i);

	memmove((void *)&posTable->p[start_b], 
		(void *)&posTable->p[end_b], 
		(posTable->used - end_b));

	for (; end_b < posTable->used; start++,end_b += sizeof(int))
		*((int *)(posTable->p) + start +1) -= decr;
		
	posTable->used -= total_bytes;
}

/**************************************************************
		_OlTLInsertString

***************************************************************/
WidgetInternal void
_OlTLInsertString(TLBuffer *b, OlStr string, int pos, OlStrRep format,
			PositionTable *posTable)
			/* PositionTable is for mb_format ... we might consider
			 * passing in the widget as a param to avoid too many
			 args */
{
	int start_pos, num_bytes;


	switch(format) {
		case OL_SB_STR_REP:
			start_pos = pos;
			num_bytes = strlen(string); /* returns type of 
							size_t */
			break;
		case OL_MB_STR_REP:
			start_pos = *((int *)(posTable->p)+pos);
			num_bytes = strlen(string); 
			break;
		case OL_WC_STR_REP:
			start_pos = pos * sizeof(wchar_t);
			num_bytes = wslen((wchar_t *)string)  * sizeof(wchar_t);
			break;
	}

	_OlTLInsertBytes(b, (char *)string, start_pos, num_bytes);
	if (format == OL_MB_STR_REP) {
		InsertPos(posTable, pos, string);
	}
}

/**************************************************************
		_OlTLDeleteString

***************************************************************/
WidgetInternal void
_OlTLDeleteString(TLBuffer *b, int start, int end, OlStrRep format,
			PositionTable *posTable)
{
	int start_pos, end_pos;

	switch(format) {
		case OL_SB_STR_REP:
			start_pos = start;
			end_pos = end;
			break;
 		case OL_MB_STR_REP:
			start_pos = *((int *)(posTable->p)+start);
			end_pos = *((int *)(posTable->p)+end);
			break;
		case OL_WC_STR_REP:
			start_pos = start * sizeof(wchar_t);
			end_pos = end * sizeof(wchar_t);
			break;
	}
	_OlTLDeleteBytes( b, start_pos, end_pos);
	if (format == OL_MB_STR_REP)
		DeletePos(posTable, start, end);
}

/**************************************************************
		_OlTLReplaceString

  Delete stuff between start & end. Insert "string" @ "start" 
***************************************************************/
WidgetInternal Boolean
_OlTLReplaceString(TextLineWidget w, int start, int  end, OlStr string)
{
	TextLinePart *tlp = &w->textLine;
	TLBuffer *b = tlp->buffer;
	OlStrRep format = w->primitive.text_format;
	PositionTable *pt = tlp->pos_table;
	UndoBuffer *u = &tlp->undo_buffer;
	int num_insert_chars;

	if (start < 0 || start > end ||  end > tlp->num_chars)
		return False;
	
	num_insert_chars = _OLStrNumChars(format, string);

/*	While Freeing the Undo structs, check for the 
	"string" field to be a NULL POINTER before Freeing it.
	If its a NULL POINTER , then Do NOT Free it ... as it's
	actually a pointer to a static variable
*/
	if (_OLStrCmp(format, u->string, _OLStrEmptyString(format)) != 0)
		XtFree(u->string);
	u->start = start;
	u->end = start + num_insert_chars;
	
	if (start != end) {

/* NOTE: VERY IMPORTANT ASSUMPTION:
	GetSubString(start, end) returns characters betwen start &
	end INCLUSIVEely as spec'ed. 
*/
		if ((u->string= _OlTLGetSubString(b,start, end-1,format,pt)) ==(OlStr)NULL)
			u->string = _OLStrEmptyString(format);
		_OlTLDeleteString(b, start, end, format, pt);
	}
	else {
		u->string = _OLStrEmptyString(format);
	}

	if (_OLStrCmp(format, string, _OLStrEmptyString(format)) != 0)
		_OlTLInsertString(b, string, start, format, pt);

	tlp->num_chars += (num_insert_chars - (end - start));
	return True;
}

/**************************************************************
		_OlTLGetSubString

  Error Checking NOT done here ... the "start" & "end" values are
  assumed to be validity-checked by the caller ...
 
***************************************************************/
WidgetInternal OlStr
_OlTLGetSubString(TLBuffer *b, int start, int end, OlStrRep format,
		PositionTable *posTable)
{
	static char *p;
	unsigned int num_bytes;
	unsigned int length_of_null = 1;

	end++;
	switch(format) {
 		case OL_MB_STR_REP:
			start = *((int *)(posTable->p)+start);
			end = *((int *)(posTable->p)+end);
			break;
		case OL_WC_STR_REP:
			start *= sizeof(wchar_t);
			end   *= sizeof(wchar_t);
			length_of_null = sizeof(wchar_t);
			break;
	}

	num_bytes = (unsigned)(end - start);

	if ((p = (char *)XtMalloc(num_bytes + length_of_null)) == (char *)NULL)
		return NULL;

	memcpy((void *)p, (void *)&b->p[start], num_bytes);

	if (format == OL_WC_STR_REP)
		*((wchar_t *)p + num_bytes/sizeof(wchar_t)) = L'\0';
	else
		p[num_bytes] = '\0';

	return p;
}
