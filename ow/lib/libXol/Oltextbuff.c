#pragma ident	"@(#)Oltextbuff.c	1.11	97/03/26 lib/libXol SMI"	/* olmisc:Mltextbuff.c 302.19	*/

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


/* WARNING: This file contains a mutex to protect Oltextbuffer
	data structure. You must not make any Xt calls from
	this file, else deadlock could result.
*/

#include <wctype.h>
#include <libintl.h>
#include <malloc.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <widec.h>
#ifdef MTSAFE
#include <thread.h>
#include <synch.h>
#include <assert.h>
#endif

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/Oltextbuff.h>
#include <Xol/OlI18nP.h>
#include <Xol/OlStrMthdsI.h>
#include <Xol/OpenLookP.h>
#include <Xol/buffutil.h>
#include <Xol/memutil.h>
#include <Xol/regexp.h>
#include <Xol/txtbufCNA.h>


#define PQLIMIT 24 
#ifdef MTSAFE
#define NO_THREAD 0
#define LockBuffer(text)	LockBuffer(text)
#define UnlockBuffer(text)		UnlockBuffer(text)
#else
#define LockBuffer(text)
#define UnlockBuffer(text)
#endif

typedef struct _OlPage {
	TextPosition		bytes;
	TextPosition		chars;
	TextLine		lines;
	TextPage		qpos;
	BlockTable*		dpos;
}			OlPage;

typedef Bufferof(TextPosition) PositionTable;

typedef struct _OlLine {
	TextPage		pageindex;
	PositionTable*		cpos;
	Buffer*			buffer;
	unsigned long		userData;
}			OlLine;

typedef Bufferof(OlLine)	OlLineTable;
typedef Bufferof(OlPage)	OlPageTable;

typedef struct _OlTextBuffer {
	char*			filename;
	FILE*			tempfile;
	TextBlock		blockcnt;
	TextBlock		blocksize;
	OlLineTable		lines;
	OlPageTable		pages;
	BlockTable*		free_list;
	PageQueue		pqueue[PQLIMIT];
	TextPage		pagecount;
	TextPage		pageref;
	TextPage		curpageno;
	Buffer*			buffer;
	PositionTable*		cpos;
	Boolean			dirty;
	TextFileStatus		status;
	int			refcount;
	TextUpdateCallback*	update;
	OlTextUndoItem		deleted;
	OlTextUndoItem		insert;
	OlStrRep		strrep;
	OlStrWordDefFunc	word_def_func;
	OlStrScanDefFunc	fwd_scan_func;
	OlStrScanDefFunc	bwd_scan_func;
	TextPosition		textblockavailable;
#ifdef	MTSAFE
	mutex_t			lock;		
	thread_t		thread_id;
	cond_t			cond_var;
	int			recursion;
#endif
	XtPointer		extension;
}			OlTextBuffer;



typedef struct _OlBufferList {
	Buffer*			buffer;
	PositionTable*		cpos;
	struct _OlBufferList*	next;
}			OlBufferList,* OlBufferListPtr;


static OlBufferListPtr OlGetBufferListFromString(OlStrRep, OlStr, int *);
static OlBufferListPtr OlGetBufferListFromFile(OlStrRep, FILE *, 
						char *,int *);

static EditResult OlInsertLineIntoTextBuffer(OlTextBuffer *,
						TextLine,
						Buffer *,
						PositionTable *);
static EditResult OlInsertLineIntoTextBufferAtLoc(OlTextBuffer *,
						TextLocation *,
						Buffer *,
						PositionTable *);
static EditResult OlInsertCharsIntoTextBufferAtLoc(OlTextBuffer *,
						TextLocation *,
						Buffer *,
						PositionTable *);
static TextPage OlSplitPage(OlTextBuffer *, TextPage, TextLine);
static void     Olprintbuffer(OlTextBuffer *);
static TextPage OlNewPage(OlTextBuffer *, TextPage);
static void     OlSwapIn(OlTextBuffer  *, TextPage);
static void     OlSwapOut(OlTextBuffer *, TextPage pageindex);
static TextLine OlFirstLineOfPage(OlTextBuffer *, TextPage);
static void     OlAllocateBlock(OlTextBuffer *, TextPage);
static void     OlFreeBlock(OlTextBuffer *, TextPage, TextBlock);
static EditResult OlInsertBlockIntoTextBuffer(OlTextBuffer *,
						TextLocation *,
						OlStr string);
static EditResult 
OlDeleteBlockInTextBuffer(OlTextBuffer *, TextLocation *, TextLocation *);

static int OlInsertBytesIntoBuffer(Buffer *, Buffer *, int, int); 
static int OlInsertOffsetsIntoPosTab(PositionTable *, PositionTable *, int); 


static TextPosition OlGetCharOffset(OlStr, OlStr, OlStrRep);

static EditResult OlDeleteLineInTextBuffer(OlTextBuffer *, TextLine);
static void OlInitTextUndoList(OlTextBuffer *);
static void OlFreeTextUndoList(OlTextBuffer *);

static Boolean is_a_SB_word_char(OlStr);
static Boolean is_a_MB_word_char(OlStr);
static Boolean is_a_WC_word_char(OlStr);

static OlStr    SB_fwd_scan_func(OlStr, OlStr, OlStr);
static OlStr    MB_fwd_scan_func(OlStr, OlStr, OlStr);
static OlStr    WC_fwd_scan_func(OlStr, OlStr, OlStr);

static OlStr    SB_bwd_scan_func(OlStr, OlStr, OlStr);
static OlStr    MB_bwd_scan_func(OlStr, OlStr, OlStr);
static OlStr    WC_bwd_scan_func(OlStr, OlStr, OlStr);


/* forward scan definitions */
static OlStrScanDefFunc mbfwd = MB_fwd_scan_func;
static OlStrScanDefFunc sbfwd = SB_fwd_scan_func;
static OlStrScanDefFunc wcfwd = WC_fwd_scan_func;

/* backward scan definitions */
static OlStrScanDefFunc mbbwd = MB_bwd_scan_func;
static OlStrScanDefFunc sbbwd = SB_bwd_scan_func;
static OlStrScanDefFunc wcbwd = WC_bwd_scan_func;

/* is a character in word definitions */
static OlStrWordDefFunc ismbwrdc = is_a_MB_word_char;
static OlStrWordDefFunc issbwrdc = is_a_SB_word_char;
static OlStrWordDefFunc iswcwrdc = is_a_WC_word_char;

/* constants */
static const long seek_set = 0; 
static const char mbnewline = '\n';
static const size_t mbnewline_len = 1;
static const size_t mbnull_len = 1;
static const char mbnull = '\0';
static const int nbuf = 4; /* must be greater than or equal to 2 */
static const int linsiz = 64;

static const size_t ftalloc = 8;
static const size_t ltalloc = 2;
static const size_t ptalloc = 4;
static const size_t btalloc = 8;
static const size_t maxlpp = 96;
static const size_t blocksize = 2048;
static const size_t lnincre = 24;
static const size_t errmsg_siz = 128;
static const wchar_t wcnewline = L'\n';
static const wchar_t wcnull = L'\0';

static char *empty_string = "";
static wchar_t *wc_empty_string = L"";

#ifdef MTSAFE
static void 
LockBuffer(OlTextBufferPtr text)
{
	mutex_lock(&text->lock);
	if(text->thread_id == thr_self()) {
		text->recursion++;
		mutex_unlock(&text->lock);
		return;
	}

	if(text->thread_id == NO_THREAD) {
		assert(text->recursion == 0);
		text-> thread_id = thr_self();
		mutex_unlock(&text->lock);
		return;
	} 

	while(text->thread_id != NO_THREAD)
		cond_wait(&text->cond_var, &text->lock);	

	assert(text->thread_id == NO_THREAD);
	text->thread_id = thr_self();
	mutex_unlock(&text->lock);
	
}

static void
UnlockBuffer(OlTextBufferPtr text)
{
	mutex_lock(&text->lock);
	assert(text->thread_id == thr_self());
	if(text->recursion == 0) 
		text->thread_id = NO_THREAD;
	else
		text->recursion--;
	mutex_unlock(&text->lock);
}
#endif
		

/*
 * OlAllocateTextBuffer
 *
 * The \fIOlAllocateTextBuffer\fR function is used to allocate a new 
 * OlTextBuffer.
 * After it allocates the structure itself, initializes
 * the members of the structure, allocating storage, setting
 * initial values, etc.
 * The routine also registers the update function provided by the caller.
 * This function normally need not be called by an application developer 
 * since the \fIOlReadFileIntoTextBuffer\fR and
 * \fIOlReadStringIntoTextBuffer\fR functions call this routine
 * before starting their operation.
 * The routine returns a pointer to the allocated OlTextBuffer.
 *
 * The \fIOlFreeTextBuffer\fR function should be used to deallocate
 * the storage allocated by this routine.
 *
 * See also:
 *
 * OlFreeTextBuffer(3), OlReadFileIntoTextBuffer(4), 
 * OlReadStringIntoTextBuffer(4)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern OlTextBufferPtr
OlAllocateTextBuffer(OlStrRep strrep, 
			char *filename, 
			TextUpdateFunction f,
			XtPointer d)
{

OlTextBufferPtr text = (OlTextBufferPtr) malloc(sizeof(OlTextBuffer));

text-> filename = filename == NULL ?
       			NULL : strcpy(malloc((unsigned)
					(strlen(filename) + 1)), filename);
text-> tempfile      = NULL;
text-> blockcnt      = (TextBlock)0;
text-> lines.used    = (TextLine)0;
text-> lines.size    = ltalloc;
text-> lines.esize   = sizeof(OlLine);
text-> lines.p       = (OlLine *) malloc((unsigned)(sizeof(OlLine)*ltalloc));
text-> pages.used    = (TextPage)0;
text-> pages.size    = ptalloc;
text-> pages.esize   = sizeof(OlPage);
text-> pages.p       = (OlPage *)malloc((unsigned)(sizeof(OlPage) * ptalloc));
text-> free_list     = 
	(BlockTable *)AllocateBuffer(sizeof(TextBlock), ftalloc);

text-> pagecount     = (TextPage)0;
text-> pageref       = (TextPage)0;
text-> curpageno     = OlNewPage(text, -1);
text-> buffer        = NULL; 
text-> cpos	     = NULL; /*used to store byte offsets in Multi-byte*/ 
text-> dirty         = FALSE;
text-> status        = NOTOPEN;
text-> update        = (TextUpdateCallback *)NULL;
text-> refcount      = 0;
text-> textblockavailable = 0;

text-> strrep    = strrep;

switch(text-> strrep) {
	case OL_SB_STR_REP:
		text-> word_def_func = issbwrdc;
		text-> fwd_scan_func = sbfwd;
		text-> bwd_scan_func = sbbwd;
		break;
	case OL_MB_STR_REP:
		text-> word_def_func = ismbwrdc;
		text-> fwd_scan_func = mbfwd;
		text-> bwd_scan_func = mbbwd;
		break;
	case OL_WC_STR_REP:
		text-> word_def_func = iswcwrdc;
		text-> fwd_scan_func = wcfwd;
		text-> bwd_scan_func = wcbwd;
		break;
}

#ifdef	MTSAFE
mutex_init(&text->lock, USYNC_THREAD, (void *)NULL);
cond_init(&text->cond_var, USYNC_PROCESS, NULL);
text->thread_id = NO_THREAD;
text->recursion =	0; 
#endif

text-> extension     = NULL;

OlRegisterTextBufferUpdate(text, f, d);
OlInitTextUndoList(text);

return (text);

} /* end of OlAllocateTextBuffer */

extern int
OlLinesInTextBuffer(OlTextBuffer *text)
{
int retval;

	LockBuffer(text);
	retval = ((int)text->lines.used);
	UnlockBuffer(text);
	return retval;
}

/*
 * OlFreeTextBuffer
 *
 * The \fIOlFreeTextBuffer\fR procedure is used to deallocate storage
 * associated with a given OlTextBuffer.  Note: the storage is not
 * actually freed if the OlTextBuffer is still associated with other
 * update function/data pairs.
 *
 * See also:
 *
 * OlAllocateTextBuffer(4), OlRegisterTextBufferUpdate(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern void
OlFreeTextBuffer(OlTextBufferPtr text,
		 TextUpdateFunction f,
		 XtPointer d		)
{
TextPage i;
TextLine j;
TextPage pageindex;

(void) OlUnregisterTextBufferUpdate(text, f, d);


if (text-> refcount == 0)
   {
#ifdef MTSAFE
cond_destroy(&text->cond_var);
mutex_destroy(&text->lock);
#endif

   for (i = 0; i < text-> lines.used; i++)
      {
      if (text-> lines.p[i].buffer-> p != NULL)
         FREE(text-> lines.p[i].buffer-> p);
      FREE((char *)text-> lines.p[i].buffer);
      if(text->lines.p[i].cpos->p != NULL) 
			/* this is non NULL in MB case */
      	FREE((char *)text->lines.p[i].cpos->p);
      FREE((char *)text->lines.p[i].cpos);
      }

   for (i = 0; i < text-> pages.used; i++)
      if (text-> pages.p[i].dpos != NULL)
         FreeBuffer((Buffer *)text-> pages.p[i].dpos);

   FREE((char *)text-> lines.p);
   FREE((char *)text-> pages.p);
   if (text-> free_list);
      FreeBuffer((Buffer *)text-> free_list);

   if (text-> tempfile != NULL)
      (void) fclose(text-> tempfile);
   if (text-> filename != NULL)
      FREE(text-> filename);

   if (text-> insert.string != NULL)
      FREE(text-> insert.string);
   if (text-> deleted.string != NULL)
      FREE(text-> deleted.string);

   FREE((char *)text);
   }

} /* OlFreeTextBuffer */

/*
 * OlReadStringIntoTextBuffer
 *
 * The \fIOlReadStringIntoTextBuffer\fR function is used to copy the
 * given \fIstring\fR into a newly allocated OlTextBuffer.  The
 * supplied TextUpdateFunction and data pointer are associated
 * with this OlTextBuffer.
 *
 * See also:
 *
 * OlReadFileIntoTextBuffer(4)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern OlTextBufferPtr
OlReadStringIntoTextBuffer(OlStrRep strrep,
			   OlStr    string,
			   TextUpdateFunction f,
			   XtPointer 	d	)

{
OlTextBufferPtr text = OlAllocateTextBuffer(strrep,NULL, f, d);
int num_lines;
int nchars;
int i = 0;

if (string != NULL) {
   OlBufferListPtr head, cur, temp;

   LockBuffer(text);
   head = cur = OlGetBufferListFromString(strrep, string, &num_lines);
   if(head && num_lines) {
	/* Allocate Lines in Line Table*/
	GrowBuffer((Buffer *)&text->lines,num_lines);
	for(i=0; i < num_lines; i++) {
		/* cur->buffer should not be freed */
		OlInsertLineIntoTextBuffer(text,text->lines.used,
						cur->buffer, cur->cpos);
		temp = cur;
		cur = cur->next;
		FREE((char *)temp);/* free buffer list entry, but not temp->buffer 
				or temp->cpos */
	} /* for */

   } /* if */

   	(void) OlGetTextBufferStringAtLine(text, 0, (TextLocation *)NULL);

  UnlockBuffer(text);
} /* if string != NULL */

return (text);

} /* end of OlReadStringIntoTextBuffer */


/* 
 * This function returns a linked list of line buffers
 * derived from the the file.
 * See Also OlGetBufferListFromString(3).
 */

static OlBufferListPtr OlGetBufferListFromFile(OlStrRep strrep,
						FILE *fp,
						char *filename,	
						int *num_lines)
{
OlBufferListPtr head, cur;
PositionTable *cpos;
Buffer *buffer;
unsigned char *fbuf; 
unsigned char *fbp; /* file buffer pointer */
size_t bytes_read;
size_t bufsiz = nbuf*BUFSIZ; /* maximum buffer size avilable */
Boolean end_of_file = FALSE;
char *errbuf;

   *num_lines = 0;
   if(fp == (FILE *)NULL)
	return((OlBufferListPtr)NULL);

   fbuf = (unsigned char *)malloc(bufsiz*sizeof(unsigned char));
   errbuf = (char *)malloc(errmsg_siz);

   /* initialize file input buffer */
   bytes_read = fread((XtPointer)fbuf, (size_t)1,bufsiz, fp);

   if(!bytes_read && ferror(fp)) { /* error */
	snprintf(errbuf, errmsg_siz,dgettext(OlMsgsDomain,
			"File read error in file: %s\n"),filename);
	OlWarning(errbuf);
	return((OlBufferListPtr)NULL);
   } else if(!bytes_read && feof(fp)) /* end of file */ 
		return((OlBufferListPtr)NULL);

   bufsiz = bytes_read; /* set actual buffer size */
   fbp = fbuf; /* set pointer to buffer */

   /* allocate buffers */
   head = cur = (OlBufferListPtr)
		calloc((unsigned)1,(unsigned)sizeof(OlBufferList));

   /* cpos buffer stores  byte offsets for multi-byte chars ;
   the bytes offsets are indexed by char offsets.  For other text formats
	cpos->used reflects number of chars in the line but
       text->cpos->p is NULL */

   cpos = (PositionTable *)AllocateBuffer(sizeof(TextPosition),0);
		
  /* buffer stores chars in a line */
   buffer = AllocateBuffer(sizeof(char),0);	

   	switch(strrep) {
	case OL_SB_STR_REP:
		while(!end_of_file){ /* while not end of file */
		Boolean end_of_buffer = FALSE;

		while(!end_of_buffer) { /* extract lines from buffer */
		size_t  num_bytes_in_line = (size_t)0;
		Boolean in_line = TRUE;
		unsigned char  *b = fbp;
		unsigned char  *p = fbp;
	
		/* extract line loop */
		while(p != (unsigned char *)NULL && num_bytes_in_line
								< bufsiz) {
		if( *p == '\n')  {  /* new line */
			in_line = FALSE;
			++num_bytes_in_line;
			p = NULL;
		} else { /* not a newline */
			num_bytes_in_line++;
			p++;
		  }

		} /* end of while loop:  extract line loop */

		if(num_bytes_in_line == bufsiz)
			end_of_buffer = TRUE;

		/* adjust remaining buffer size */
			bufsiz -= num_bytes_in_line;

		GrowBuffer(buffer,num_bytes_in_line);
		memmove((XtPointer)&buffer->p[buffer->used], 
				(XtPointer)b, num_bytes_in_line);
			
		buffer->used += num_bytes_in_line;
		cpos->used += num_bytes_in_line;

		if(!in_line) { /* a valid line found */
			buffer->p[buffer->used - 1] = '\0';
			(*num_lines)++;
			cur->buffer = buffer;
			cur->cpos = cpos;
			if(!end_of_buffer) { /* more lines in buffer */
   				cur->next = (OlBufferListPtr)
					calloc((unsigned)1,
					(unsigned)sizeof(OlBufferList));
				cur    = cur->next;
   				buffer = AllocateBuffer(sizeof(char),0);
				cpos = (PositionTable *)
					AllocateBuffer(
						sizeof(TextPosition),0);
			} /* not end of buffer */

		} /* not in line */

		if(end_of_buffer) { /* read into buffer from file */
			fbp = fbuf;
   			bytes_read = fread(fbuf, (size_t)1,nbuf*BUFSIZ, fp);

   			if(!bytes_read && ferror(fp)) {  /* error */
				snprintf(errbuf, errmsg_siz,dgettext(OlMsgsDomain,
				  	"File read error in file: %s\n"),
								filename);
				OlWarning(errbuf);
				return((OlBufferListPtr)NULL);
   			}  else if(!bytes_read && feof(fp))
						end_of_file = TRUE;

		if(!end_of_file) { /* more lines in file*/

 		    if(!in_line) {
   			cur->next = (OlBufferListPtr) calloc((unsigned)1,
				      (unsigned)sizeof(OlBufferList));
			cur    = cur->next;
   			buffer = AllocateBuffer(sizeof(char),0);
			cpos = (PositionTable *)AllocateBuffer(
						sizeof(TextPosition),0);
		    }
		    bufsiz = bytes_read;

		} else {
			/* file ends */

			/* check for being in the middle of a line */

			if(in_line) { /* in the middle of a line */
				GrowBuffer(buffer,1);

				/* append NULL character */
				buffer->p[buffer->used] = '\0';
				buffer->used++;
				cpos->used++;
				cur->buffer = buffer;
				cur->cpos = cpos;

				++*num_lines;
			} /*in line */

			} /* else not end of file */

		} else /* more stuff in buffer yet, set buffer pointer
			  to the beginning of next line */
			fbp += num_bytes_in_line;

		} /* while  not end of buffer */

		} /* while not end of file */
		break;

	case OL_MB_STR_REP: /* Multi-Byte representation */
   		GrowBuffer((Buffer *)cpos, linsiz);
		
		while(!end_of_file){ /* while not end of file */
		Boolean end_of_buffer = FALSE;

		while(!end_of_buffer) { /* extract lines from buffer */
		size_t  num_bytes_in_mbchar = 0; 
		size_t  num_bytes_in_line = 0;
		Boolean in_line = TRUE;
		Boolean in_char = FALSE;
		unsigned char  *b = fbp;
		unsigned char  *p = fbp;
	
		/* extract line loop */
		while(p != (unsigned char *)NULL  && 
			num_bytes_in_line < bufsiz) { 

		size_t bufleft = (size_t)((int)bufsiz - 
						(int)num_bytes_in_line);

		num_bytes_in_mbchar =mblen((char *)p, bufleft);

		if(num_bytes_in_mbchar == (size_t)-1) {
			/* we have a split mb char*/  
			in_char = TRUE;
			in_line = TRUE;
			p = NULL;
		} else if( num_bytes_in_mbchar == mbnewline_len  &&
			   *p == mbnewline ) { 
			/* found a newline mb char */

			/* save the byte offset */
			if(cpos->used < cpos->size) { 
				/*  CTE# 503772 BID# 1207648
				 *  Include buffer->used in the calculation
				 *  in case line spans buffers.
				 */
				cpos->p[cpos->used++] = 
					buffer->used + num_bytes_in_line;
			} else {
				GrowBuffer((Buffer *)cpos,linsiz);
				/*  CTE# 503772 BID# 1207648
				 *  Include buffer->used in the calculation
				 *  in case line spans buffers.
				 */
				cpos->p[cpos->used++] = 
					buffer->used + num_bytes_in_line;
			}

			num_bytes_in_line += num_bytes_in_mbchar;
			in_line = FALSE;
			in_char = FALSE;
			p = NULL;
		} else { /* not a mbnewline or split mb char */

			/* save the byte offset */
			if(cpos->used < cpos->size) { 
				/*  CTE# 503772 BID# 1207648
				 *  Include buffer->used in the calculation
				 *  in case line spans buffers.
				 */
				cpos->p[cpos->used++] = 
					buffer->used + num_bytes_in_line;
			} else {
				GrowBuffer((Buffer *)cpos,linsiz);
				/*  CTE# 503772 BID# 1207648
				 *  Include buffer->used in the calculation
				 *  in case line spans buffers.
				 */
				cpos->p[cpos->used++] = 
					buffer->used + num_bytes_in_line;
			}

			p += num_bytes_in_mbchar;
			num_bytes_in_line += num_bytes_in_mbchar;
		   } /* else */

		} /* end of while loop:  extract line loop */

		if(num_bytes_in_line >= bufsiz || in_char)
			end_of_buffer = TRUE;

		/* adjust remaining buffer size */
			bufsiz -= num_bytes_in_line;

		GrowBuffer(buffer,num_bytes_in_line);
		memmove((XtPointer)&buffer->p[buffer->used], 
				(XtPointer)b, num_bytes_in_line);
			
		buffer->used += num_bytes_in_line;

		if(!in_line) { /* a valid line found */

			/* replace newline by null */
			buffer->p[buffer->used - mbnull_len] = mbnull; 
			(*num_lines)++;
			cur->buffer = buffer;
			cur->cpos   = cpos;

			if(!end_of_buffer) { /* more lines in buffer */
   				cur->next = (OlBufferListPtr)
					calloc((unsigned)1,
					(unsigned)sizeof(OlBufferList));
				cur    = cur->next;
   				buffer = AllocateBuffer(sizeof(char),0);
   				cpos = (PositionTable *)
						AllocateBuffer(
						sizeof(TextPosition), 
								linsiz);
			} /* not end of buffer */

		} /* not in line */

		if(end_of_buffer) { /* read into buffer from file */

			if(!in_char) { /* no split multi-byte char */

				fbp = fbuf;
   				bytes_read = 
					fread(fbuf, (size_t)1,
							nbuf*BUFSIZ, fp);
   				if(!bytes_read && ferror(fp)) {  /* error */
				  snprintf(errbuf, errmsg_siz,dgettext(OlMsgsDomain,
					  "File read error in file: %s\n"),
								filename);
					OlWarning(errbuf);
					return((OlBufferListPtr)NULL);
   				}  else if(!bytes_read && feof(fp))
						end_of_file = TRUE;
				bufsiz = bytes_read;
			} else {  
				/* split a chracter: move last BUFSIZ block
			     		to first BUFSIZ block */

				fbp = &fbuf[BUFSIZ - bufsiz];
				memmove((XtPointer)fbuf,&fbuf[(nbuf-1)
						*BUFSIZ],(size_t)BUFSIZ);
   				bytes_read = 
					fread(&fbuf[BUFSIZ],(size_t)1,
							(nbuf-1)*BUFSIZ, fp);
   				if(!bytes_read && ferror(fp)) {  /* error */
				  snprintf(errbuf, errmsg_siz,dgettext(OlMsgsDomain,
					  "File read error in file: %s\n"),
								filename);
					OlWarning(errbuf);
					return((OlBufferListPtr)NULL);
   				}  else if(!bytes_read && feof(fp))
						end_of_file = TRUE;
				bufsiz += bytes_read;
			}

			if(!end_of_file) { /* more lines in file*/

				if(!in_line) { /* no split mb char */
   					cur->next = (OlBufferListPtr)
						calloc((unsigned)1,
					      (unsigned)sizeof(OlBufferList));
					cur   = cur->next;
   					buffer = 
					    AllocateBuffer(sizeof(char),0);
   					cpos = (PositionTable *)
						AllocateBuffer(
						sizeof(TextPosition),linsiz);
				} 

			} else {
				/* file ends */

				/* check for being in the middle of a line */

				if(in_line) { /* in the middle of a line */
					GrowBuffer(buffer,mbnull_len);
					buffer->p[buffer->used] = mbnull;
					buffer->used += mbnull_len;

					/* save the byte offset */
					if(cpos->used < cpos->size) { 
					cpos->p[cpos->used++] = 
						num_bytes_in_line;
					} else {
					GrowBuffer((Buffer *)cpos,linsiz);
					cpos->p[cpos->used++] = 
						num_bytes_in_line;
					}

					cur->buffer = buffer;
					cur->cpos = cpos;
					++*num_lines;

				} /*in line */

			} /* else not end of file */

		} else /* more stuff in buffer yet, set buffer pointer
			  to the beginning of next line */
			fbp += num_bytes_in_line;

		} /* while  not end of buffer */

		} /* while not end of file */
		break;
	case OL_WC_STR_REP:
		while(!end_of_file){ /* while not end of file */
		Boolean end_of_buffer = FALSE;

		while(!end_of_buffer) { /* extract lines from buffer */
		size_t  num_bytes_in_mbchar = 0; 
		size_t  mbchars = 0;
		size_t  num_bytes_in_line = 0;
		Boolean in_line = TRUE;
		Boolean in_char = FALSE;
		unsigned char  *b = fbp;
		unsigned char *p  = fbp;
		wchar_t *w;
	
		/* extract line loop */
		while(p != (unsigned char *)NULL  && 
					num_bytes_in_line < bufsiz) {
		size_t bufleft = (size_t)((int)bufsiz -
						(int)num_bytes_in_line);

		num_bytes_in_mbchar =mblen((char *)p,bufleft);

		if(num_bytes_in_mbchar == (size_t)-1) {
			/* we have a split mb char*/  
			in_char = TRUE;
			in_line = TRUE;
			p = NULL;
		} else if( num_bytes_in_mbchar == mbnewline_len  &&
				*p == mbnewline ) { 
			/* found a newline mb char */
			mbchars++;
			cpos->used++;
			num_bytes_in_line += num_bytes_in_mbchar;
			in_line = FALSE;
			in_char = FALSE;
			p = NULL;
		} else { /* not a mbnewline or split mb char */
			mbchars++;
			cpos->used++;
			p += num_bytes_in_mbchar;
			num_bytes_in_line += num_bytes_in_mbchar;
		  } /* else */

		} /* end of while:  extract line loop */

		if(num_bytes_in_line >= bufsiz || in_char)
			end_of_buffer = TRUE;

		/* adjust remaining buffer size */
			bufsiz -= num_bytes_in_line;

		/* convert to wide char */
		{
		size_t nwc = 0;
		size_t num_wc_bytes = 0;

		w = (wchar_t *)malloc((unsigned)(sizeof(wchar_t)*mbchars));
		if((nwc = mbstowcs(w,(const char *)b,(size_t)mbchars)) == 
							(size_t)-1) {
				fprintf(stderr,
					"OlGetBufferListFromFile:mbstowcs: \
invalid multi-byte character\n");
				return((OlBufferListPtr)NULL);
			}

		num_wc_bytes = nwc*sizeof(wchar_t);
		GrowBuffer(buffer,num_wc_bytes);
		memmove((XtPointer)&buffer->p[buffer->used], 
					(XtPointer)w,num_wc_bytes);
			
		buffer->used += num_wc_bytes;

		} /* end block */

		if(!in_line) { /* a valid line found */
			/* replace newline by null */
			memmove((XtPointer)
				&(buffer->p[buffer->used - sizeof(wchar_t)]),
			     		(XtPointer)&wcnull, sizeof(wchar_t));
			(*num_lines)++;
			cur->buffer = buffer;
			cur->cpos = cpos;
			if(!end_of_buffer) { /* more lines in buffer */
   				cur->next = (OlBufferListPtr)
					calloc((unsigned)1,
					(unsigned)sizeof(OlBufferList));
				cur    = cur->next;
   				buffer = AllocateBuffer(sizeof(char),0);
				cpos = (PositionTable *)
					AllocateBuffer(
						sizeof(TextPosition),0);
			} /* not end of buffer */

		} /* not in line */

		if(end_of_buffer) { /* read into buffer from file */

			if(!in_char) { /* no split multi-byte char */

				fbp = fbuf;
   				bytes_read = 
					fread(fbuf, (size_t)1,
							nbuf*BUFSIZ, fp);
   				if(!bytes_read && ferror(fp)) {  /* error */
				  snprintf(errbuf, errmsg_siz,dgettext(OlMsgsDomain,
					  "File read error in file: %s\n"),
								filename);
					OlWarning(errbuf);
					return((OlBufferListPtr)NULL);
   				}  else if(!bytes_read && feof(fp)) 
						end_of_file = TRUE;
				bufsiz = bytes_read;
			} else {  
				/* split a chracter: move last BUFSIZ block
			     		to first BUFSIZ block */

				fbp = &fbuf[BUFSIZ - bufsiz];
				memmove((XtPointer)fbuf,
				     &fbuf[(nbuf-1)*BUFSIZ],(size_t)BUFSIZ);
   				bytes_read = 
					fread(&fbuf[BUFSIZ],(size_t)1,
							(nbuf-1)*BUFSIZ, fp);
   				if(!bytes_read && ferror(fp)) {  /* error */
				  snprintf(errbuf, errmsg_siz,dgettext(OlMsgsDomain,
					  "File read error in file: %s\n"),
								filename);
					OlWarning(errbuf);
					return((OlBufferListPtr)NULL);
   				}  else if(!bytes_read && feof(fp))
						end_of_file = TRUE;
				bufsiz += bytes_read;
			}

			if(!end_of_file) { /* more lines in file*/

				if(!in_line) { /* no split mb char */
   					cur->next = (OlBufferListPtr)
						calloc((unsigned)1,
					        (unsigned)sizeof(
							OlBufferList));
					cur    = cur->next;
   					buffer = 
					    AllocateBuffer(sizeof(char),0);
					cpos = (PositionTable *)
					AllocateBuffer(sizeof(TextPosition)
								,0);
				}  

			} else {
				/* file ends */

				/* check for being in the middle of a line */

				if(in_line) { /* in the middle of a line */
					GrowBuffer(buffer,sizeof(wchar_t)); 
					/* append null */
					memmove((XtPointer)
						&(buffer->p[buffer->used]),
			     			(XtPointer)&wcnull, 
						sizeof(wchar_t));
						buffer->used += 
							sizeof(wchar_t);
						cpos->used++;
					cur->buffer = buffer;
					cur->cpos = cpos;
					++*num_lines;
				} /*in line */

			} /* else not end of file */

		} else /* more stuff in buffer yet, set buffer pointer
			  to the beginning of next line */
			fbp += num_bytes_in_line;

		} /* while  not end of buffer */

		} /* while not end of file */
		break;

	default:
		FREE((char *)head);
		return((OlBufferListPtr)NULL);
	} /* switch */

FREE((char *)fbuf);
FREE(errbuf);
return(head);
}

/*
 *	This function converts the string into a linked list
 * of buffers, with each buffer essentially associated with a line.
 * See Also OlGetBufferListFromFile(4).
 */

static OlBufferListPtr OlGetBufferListFromString(OlStrRep strrep,
					OlStr	string,
					int 	*num_lines)
{
OlBufferListPtr head, cur;
Buffer *buffer;
size_t ptr_inc;
Boolean end_of_string = FALSE;
PositionTable *cpos;



   *num_lines = 0;

   if(string == NULL)
	return((OlBufferListPtr)NULL);

   head = cur = (OlBufferListPtr)
		calloc((unsigned)1,(unsigned)sizeof(OlBufferList));

   cpos = (PositionTable *) AllocateBuffer( sizeof(TextPosition),0);

   switch(strrep) {
	case OL_MB_STR_REP: /* Multi-Byte representation */
		GrowBuffer((Buffer *)cpos,linsiz);

		while (!end_of_string) { /* extract next line */
		size_t  num_bytes_in_mbchar = 0; 
		size_t  num_bytes_in_line = 0;
		unsigned char  *b = (unsigned char *)string;
		unsigned char  *p = (unsigned char *)string;
		size_t str_len; 

		/* get length in bytes */
		str_len = strlen((const char *)string) + mbnull_len;

		 while(p != (unsigned char *)NULL) {

		/* line loop */

		num_bytes_in_mbchar = mblen((char *)p,
			(size_t)((int)str_len - (int)num_bytes_in_line));

		/* check for invalid multi-byte character */
		if(num_bytes_in_mbchar == (size_t)-1) { 
			OlWarning(dgettext(OlMsgsDomain,
			 "Invalid multibyte character code in string\n"));
			return((OlBufferListPtr)NULL);
		}

		/* check if it is a null character */
		if(num_bytes_in_mbchar == 0 )
			num_bytes_in_mbchar = mbnull_len;

		/* check for newline or null */
		if( ((num_bytes_in_mbchar == mbnewline_len)  &&
			*p ==mbnewline )  || 
			( (num_bytes_in_mbchar == mbnull_len)  &&
					*p == mbnull ) ) {
				/* end of line or end of string*/
				/* save the byte offset */
				if(cpos->used < cpos->size) { 
					cpos->p[cpos->used++] = 
						num_bytes_in_line;
				} else {
					GrowBuffer((Buffer *)cpos,linsiz);
					cpos->p[cpos->used++] = 
						num_bytes_in_line;
				}

				num_bytes_in_line += num_bytes_in_mbchar;
				buffer = AllocateBuffer(sizeof(char),
							num_bytes_in_line);
				memmove((XtPointer)buffer->p,(XtPointer)b, 
							num_bytes_in_line);
				buffer->used = num_bytes_in_line;


			if( (num_bytes_in_mbchar == mbnull_len)  &&
				*p == mbnull ) 
				/* end of string */
				end_of_string = TRUE;
			else
			/* replace end of line by mbnull character */
				buffer->p[buffer->used - mbnull_len]
								= mbnull;
				p = NULL;
		} else { /* not a mbnewline or end of string  */
			/* save the byte offset */
			if(cpos->used < cpos->size) { 
				cpos->p[cpos->used++] = 
					num_bytes_in_line;
			} else {
				GrowBuffer((Buffer *)cpos,linsiz);
				cpos->p[cpos->used++] = 
					num_bytes_in_line;
			}

			p += num_bytes_in_mbchar;
			num_bytes_in_line += num_bytes_in_mbchar;

		} /* else */

		} /* end of while loop */

		(*num_lines)++;
		cur->buffer = buffer;
		cur->cpos = cpos;
		if(!end_of_string) { /* more lines */
   			cur->next = (OlBufferListPtr)calloc((unsigned)1,
					(unsigned)sizeof(OlBufferList));
			cur = cur->next;
   			cpos = (PositionTable *)AllocateBuffer(
					sizeof(TextPosition), linsiz);
			string = (char *)string + num_bytes_in_line;
		} /* if */

		} /* while loop */
		break;

	case OL_WC_STR_REP: /* wide char representation */
		while (!end_of_string) { /* extract next line */
		int wcchars = 0;
		size_t num_bytes_in_line = 0;
		wchar_t *b = (wchar_t *)string;
		wchar_t *w = (wchar_t *)string;

		while(w != (wchar_t *)NULL) { 
		/* line loop */

		if( (*w == wcnewline) || (*w == wcnull) ) { 
			/* end of line or end of string*/
			wcchars++;
			num_bytes_in_line = wcchars*sizeof(wchar_t);
			buffer = AllocateBuffer(sizeof(char),
						num_bytes_in_line);
			memmove((XtPointer)buffer->p,(XtPointer)b,
						num_bytes_in_line);
			buffer->used = num_bytes_in_line;
			cpos -> used = wcchars;

			if(*w == wcnull)
				/* end of string */
				end_of_string = TRUE;
			else
			/* replace end of line by null character */
			memmove((XtPointer)&(buffer->p[buffer->used - 
				sizeof(wchar_t)]),(XtPointer)&wcnull,sizeof(wchar_t));
			w = NULL;
		} else { /* not a newline or end of string  */
			w++;
			wcchars++;
		}

		} /* end of while */
		(*num_lines)++;
		cur->buffer = buffer;
		cur->cpos = cpos;
		if(!end_of_string) { /* more lines */
   			cur->next = (OlBufferListPtr)calloc((unsigned)1,
					(unsigned)sizeof(OlBufferList));
			cur = cur->next;
   			cpos = (PositionTable *)AllocateBuffer(
					sizeof(TextPosition), 0);
			string = (char *)string + num_bytes_in_line;
		} /* if */

		} /* while loop */

		break;
	case OL_SB_STR_REP: /* wide char representation */
		while (!end_of_string) {
		size_t num_bytes_in_line = 0;
		int mbchars = 0;
		unsigned char *b = (unsigned char *)string;
		unsigned char *c = (unsigned char *)string;

		while(c != (unsigned char *)NULL) {

		if( (*c == '\n') || (*c == '\0') ) { 
			/* end of line or end of string*/
			num_bytes_in_line++; 
			buffer = AllocateBuffer(sizeof(char),
						num_bytes_in_line);
			memmove((XtPointer)buffer->p,(XtPointer)b,
						num_bytes_in_line);
			buffer->used = num_bytes_in_line;
			cpos->used = buffer->used;

			if(*c == '\0') 
				/* end of string */
				end_of_string = TRUE;
			else
			/* replace end of line by null character */
				buffer->p[buffer->used -1] = '\0';

			c = NULL;
		} else { /* not a newline or end of string  */
			c++;
			num_bytes_in_line++;
		}

		} /*end of while */

		(*num_lines)++;
		cur->buffer = buffer;
		cur->cpos = cpos;
		if(!end_of_string) { /* more lines */
   			cur->next = (OlBufferListPtr)calloc((unsigned)1,
					(unsigned)sizeof(OlBufferList));
			cur = cur->next;
   			cpos = (PositionTable *)AllocateBuffer(
					sizeof(TextPosition), 0);
			string = (char *)string + num_bytes_in_line;
		} /* if */

		} /* while loop */
		break;
	default:
		FREE((char *)head);
		return((OlBufferListPtr)NULL);
	} /* switch */

return(head);
}

/*
 * OlInsertLineIntoTextBuffer
 *
 * This function insert a given Buffer into a given OlTextBuffer at
 * a given line.  The algorithm used is:	
 *
 *  1. Check for line within range, if not return failure else continue.
 *  2. Determine if affected page has room for another line, if not
 *     allocate a new page.
 *  3. If page is not the current page then OlSwapIn the page.
 *  4. Increase the LineTable and shift down if necessary.
 *  5. Store this buffer pointer and the pageindex in the line table.
 *  6. Increment the Page bytes, chars and lines.
 *  7. Return success.
 *
 * NOTE: The 'buffer' containing the line is not copied into another
 * 	 buffer by this function. Therefore, 'buffer' should not be freed
 *	 by the caller.
 */

static EditResult
OlInsertLineIntoTextBuffer(OlTextBuffer *text,
				TextLine  at,
				Buffer *buffer,
				PositionTable *cpos)
{
TextPage page;
int savesize;

if (at < (TextLine)0 || at > text-> lines.used)
   return (EDIT_FAILURE);
else
   {
   page = (text-> lines.used ==  (TextLine)0 ? (TextPage)0 :
           text-> lines.used == at ? text-> lines.p[at - 1].pageindex :
                                     text-> lines.p[at].pageindex);

   if (text-> pages.p[page].lines == maxlpp)
      page = OlSplitPage(text, page, at);
   else
      if (text-> curpageno != page)
         OlSwapIn(text, page);

   if (text-> lines.used == text-> lines.size)
      {
      text-> lines.size *= 2;
      text-> lines.p = (OlLine *) REALLOC
         ((char *)text->lines.p,(unsigned)(text->lines.size*sizeof(OlLine)));
      }

   if (at < text-> lines.used++)
      memmove((XtPointer)&text-> lines.p[at + 1], (XtPointer)&text-> lines.p[at],
              (text-> lines.used - at - 1) * sizeof(OlLine));

   text-> lines.p[at].buffer    = buffer;
   text->lines.p[at].cpos = cpos;
   text-> lines.p[at].pageindex = page;
   text-> lines.p[at].userData  = 0L;

   text-> pages.p[page].bytes += text-> lines.p[at].buffer-> used;
   text->pages.p[page].chars += text->lines.p[at].cpos->used;
   text-> pages.p[page].lines++;

   return (EDIT_SUCCESS);
   }

} /* end of OlInsertLineIntoTextBuffer */

/*
 * OlSplitPage
 *
 */

static TextPage
OlSplitPage(OlTextBuffer *text,
		TextPage after,
		TextLine line)
{
TextPage newpage;
TextLine l;
TextLine s;

#ifdef DEBUG_OLTEXTBUFF
   Olprintbuffer(text);
   (void) fprintf(stderr,"splitting page %d\n", after);
#endif

   if (text-> curpageno != after)
      OlSwapIn(text, after);
   s = l = OlFirstLineOfPage(text, after) + maxlpp / 2;
   newpage = OlNewPage(text, after);
   while (l < text-> lines.used && text-> lines.p[l].pageindex == after)
      {
      text-> pages.p[after].bytes -= text-> lines.p[l].buffer-> used;
      text->pages.p[after].chars -= text->lines.p[l].cpos->used;
      text-> pages.p[after].lines--;
      l++;
      }
#ifdef DEBUG_OLTEXTBUFF
   (void) fprintf(stderr,"AFTER = %d NEWPAGE = %d   ...", after, newpage);
#endif
   l = s;
   while (l < text-> lines.used && text-> lines.p[l].pageindex == after)
      {
      text-> lines.p[l].pageindex = newpage;
      text-> pages.p[newpage].bytes += text-> lines.p[l].buffer-> used;
      text->pages.p[newpage].chars += text->lines.p[l].cpos->used;
      text-> pages.p[newpage].lines++;
      l++;
      }
   newpage = (text-> lines.used ==  (TextLine)0 ? (TextPage)0 :
              		text-> lines.used == line ? 
	      		text-> lines.p[line - 1].pageindex :
              		text-> lines.p[line].pageindex);

#ifdef DEBUG_OLTEXTBUFF
   (void) fprintf(stderr,"l: %d newpage = %d  curpage = %d\n", 
                  line, newpage, text-> curpageno);
   Olprintbuffer(text);
#endif
   if (text-> curpageno != newpage)
      		OlSwapIn(text, newpage);

return (newpage);

} /* end of OlSplitPage */


/*
 * Olprintbuffer
 *
 */

static void
Olprintbuffer(OlTextBuffer *	text)
{
register int i;
register int j;

(void) fprintf(stderr, " page  qpos  dpos lines   used blocks...\n");
for (i = 0; i < text-> pages.used; i++)
   {
   (void) fprintf(stderr, "%c[K%5d %5d %5d %5d %5d",
   0x1b /* esc */, i,
   text-> pages.p[i].qpos,
   text-> pages.p[i].dpos,
   text-> pages.p[i].lines,
   text-> pages.p[i].bytes);
   for (j = 0; text-> pages.p[i].dpos != NULL &&
               j < text-> pages.p[i].dpos-> used; j++)
   (void) fprintf(stderr, "\t%5d", text-> pages.p[i].dpos-> p[j]);
   (void) fprintf(stderr, "\n");
   }

(void) fprintf(stderr, " qpos  page  time\n");
for (i = 0; i < text-> pagecount; i++)
   (void) fprintf(stderr,"%5d %5d %5d\n",
   i,
   text-> pqueue[i].pageindex,
   text-> pqueue[i].timestamp);

(void) fprintf(stderr, " line  page   blen   bslen size\n");
for (i = 0; i < text-> lines.used; i++)
   {
   (void) fprintf(stderr,"%5d %5d %5d %5d %5d\n", 
                  i, 
                  text-> lines.p[i].pageindex,
                  text-> lines.p[i].buffer-> used,
                  strlen(text-> lines.p[i].buffer-> p),
                  text-> lines.p[i].buffer-> size);
   }

} /* end of Olprintbuffer */

/*
 * OlReadFileIntoTextBuffer
 *
 * The \fIOlReadFileIntoTextBuffer\fR function is used to read the
 * given \fIfile\fR into a newly allocated OlTextBuffer.  The
 * supplied TextUpdateFunction and data pointer are associated
 * with this OlTextBuffer.
 *
 * See also:
 *
 * OlReadStringIntoTextBuffer(4)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern OlTextBuffer *
OlReadFileIntoTextBuffer(OlStrRep strrep,
			char *filename, 
			TextUpdateFunction f, 
				XtPointer d)
{
FILE * fp;
OlTextBuffer * text = NULL;
TextFileStatus status = READWRITE;
OlBufferListPtr cur,temp;
int num_lines = 0;
register int i;

status = READWRITE;
if ((fp = fopen(filename, "rw")) == NULL)
   {
   status = READONLY;
   if ((fp = fopen(filename, "r")) == NULL)
      status = NEWFILE;
   }

if (fp != NULL) {

   cur = OlGetBufferListFromFile(strrep, fp, filename, &num_lines);
   if(cur && num_lines) {
   	text = OlAllocateTextBuffer(strrep, filename, f, d);
   	LockBuffer(text);
   	text-> status = status;
	/* Allocate Lines in Line Table*/
	GrowBuffer((Buffer *)&text->lines,num_lines);
	for(i=0; i < num_lines; i++) {
		OlInsertLineIntoTextBuffer(text,text->lines.used,
					cur->buffer, cur->cpos);
		temp = cur;
		cur = cur->next;
		FREE((char *)temp);/* free buffer list entry, but not temp->buffer */
	}
   	(void) OlGetTextBufferStringAtLine(text, 
					0, (TextLocation *)NULL);
   	UnlockBuffer(text);
   }

   (void) fclose(fp);
   }

return (text);

} /* end of OlReadFileIntoTextBuffer */

/*
 * OlGetTextBufferStringAtLine 
 *
 * The \fIOlGetTextBufferStringAtLine\fR function is used to retrieve
 * the contents of the given line within the OlTextBuffer.  It
 * returns a pointer to the OlStr string.  If the line
 * number is invalid a NULL pointer is returned.  
 * If a non-NULL TextLocation
 * pointer is supplied in the argument list the contents of this
 * structure are modified to reflect the values corresponding to
 * the given line.
 *
 * See also:
 *
 * OlGetTextBufferBlock(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern OlStr 
OlGetTextBufferStringAtLine(OlTextBuffer *text,
    			    TextLine	line_number, 
    			    TextLocation *location)
{
OlStr	retval;

LockBuffer(text);
if (line_number < 0 || line_number >= text-> lines.used) {
   UnlockBuffer(text);
   return (NULL);
}

if (text-> lines.p[line_number].pageindex != text-> curpageno)
   OlSwapIn(text, text-> lines.p[line_number].pageindex);

text-> buffer = text-> lines.p[line_number].buffer;
text-> cpos = text->lines.p[line_number].cpos;

if (location)
   {
   location-> line   = line_number;
   location-> offset = 0;
   location-> buffer = text-> buffer-> p;
   }

retval =  text-> buffer->p;
UnlockBuffer(text);
return retval;

} /* OlGetTextBufferStringAtLine */

/*
 * OlForwardScanTextBuffer
 *
 * The \fIOlForwardScanTextBuffer\fR function is used to scan,
 * towards the end of the buffer,
 * for a given \fIexp\fRression in the OlTextBuffer starting
 * at \fIlocation\fR.  A \fIScanResult\fR is returned which
 * indicates
 *
 * .so CWstart
 *   SCAN_NOTFOUND  The scan wrapped without finding a match.
 *   SCAN_WRAPPED   A match was found at a location \fIbefore\fP the start location.
 *   SCAN_FOUND     A match was found at a location \fIafter\fP the start location.
 *   SCAN_INVALID   Either the location or the expression was invalid.
 * .so CWend
 *
 * See also:
 *
 * OlBackwardScanTextBuffer(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern ScanResult
OlForwardScanTextBuffer(OlTextBuffer *text, 
		      OlStr	   exp,
    		      TextLocation *location)
{
OlStr c;
TextLine s = location-> line;
TextPosition lc = location->offset; /* offset in chars */
TextPosition lb; /* byte offset of location->offset */
TextPosition lb_1; /* byte offset of location->offset */
ScanResult retval = SCAN_INVALID;
OlStrScanDefFunc Strexp;

LockBuffer(text);
Strexp = text->fwd_scan_func;
switch(text->strrep) {
	case OL_MB_STR_REP:
		{
		PositionTable const *cpos = text->lines.p[s].cpos;

		if(lc > (cpos->used - 1)) {
			UnlockBuffer(text);
			return retval;
		}

		lb = cpos->p[lc];
		if(lc < (cpos->used -1))
			lb_1 = cpos->p[lc];
		else {
			if(s < text->lines.used - 1) 
				s++;
			else 
				s = 0;
			lb_1 = 0;		
		}
		}
		break;
	case OL_SB_STR_REP:
		{
		Buffer const *buffer = text->lines.p[s].buffer;

		if(lc > (buffer->used - 1) ) {
			UnlockBuffer(text);
			return retval;
		}

		lb = lc;

		if(lc < (buffer->used -1))
			lb_1 = lc;
		else {
			if( s < text->lines.used -1 )
				s++;
			else
				s = 0;
			
			lb_1 = 0;
		}
		}
		break;
	case OL_WC_STR_REP:
		{
		Buffer const *buffer = text->lines.p[s].buffer;
		TextPosition nchars  = (buffer->used)/sizeof(wchar_t);

		if(lc > (nchars - 1) ) {
			UnlockBuffer(text);
			return retval;
		}

		lb = lc*sizeof(wchar_t);

		if(lc  < (nchars - 1))
			lb_1 = (lc)*sizeof(wchar_t);
		else {
			if( s < text->lines.used -1 )
				s++;
			else
				s = 0;
			
			lb_1 = 0;
		}
		}
		break;
} /* switch */

if (exp != NULL)
   {
   retval = SCAN_FOUND;
   (void) OlGetTextBufferStringAtLine(text, s, (TextLocation *)NULL);
   if ((c = Strexp(text-> buffer-> p,
		&text-> buffer-> p[lb_1], exp)) == NULL)
      {
      for (s = location-> line + 1; s < text-> lines.used; s++)
         {
         (void) OlGetTextBufferStringAtLine(text, s, (TextLocation *)NULL);
         if ((c = Strexp(text-> buffer-> p, text-> buffer-> p, exp)) != NULL)
            break;
         }

      if (s == text-> lines.used)
         {
         retval = SCAN_WRAPPED;
         for (s = 0; s <= location-> line; s++)
            {
            (void) OlGetTextBufferStringAtLine(text, s, (TextLocation *)NULL);
            if ((c = Strexp(text-> buffer-> p,
                            text-> buffer-> p, exp)) != NULL)
               break;
            }
         if (s > location-> line || (s == location-> line &&
            (c == NULL || ((char *)c - (char *)text->buffer->p) >= lb)))
            retval = SCAN_NOTFOUND;
         }
      }
   if (retval == SCAN_FOUND || retval == SCAN_WRAPPED)
      {
      location-> line   = s;
      location-> offset = OlGetCharOffset(c,text->buffer->p,text->strrep);
      location-> buffer = text-> buffer-> p;
      }
   }

UnlockBuffer(text);
return (retval);

} /* end of OlForwardScanTextBuffer */

/*
 * OlBackwardScanTextBuffer
 *
 * The \fIOlBackwardScanTextBuffer\fR function is used to scan,
 * towards the beginning of the buffer,
 * for a given \fIexp\fRression in the OlTextBuffer starting
 * at \fIlocation\fR.  A \fIScanResult\fR is returned which
 * indicates
 *
 * .so CWstart
 *   SCAN_NOTFOUND  The scan wrapped without finding a match.
 *   SCAN_WRAPPED   A match was found at a location \fIafter\fP the start location.
 *   SCAN_FOUND     A match was found at a location \fIbefore\fP the start location.
 *   SCAN_INVALID   Either the location or the expression was invalid.
 * .so CWend
 *
 * See also:
 *
 * OlForwardScanTextBuffer(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern ScanResult
OlBackwardScanTextBuffer(OlTextBuffer *text, 
		      OlStr	   exp,
    		      TextLocation *location)
{
OlStr c;
TextLine s = location-> line;
TextPosition lc = location->offset; /* offset in chars */
TextPosition lb; /* byte offset of location->offset */
TextPosition lb_1; /* byte offset of location->offset - 1*/
ScanResult retval = SCAN_INVALID;
OlStrScanDefFunc Strrexp;

LockBuffer(text);
Strrexp = text->bwd_scan_func;
switch(text->strrep) {
	case OL_MB_STR_REP:
		{
		PositionTable const *cpos = text->lines.p[s].cpos;

		if(lc > (cpos->used -1)) {
			UnlockBuffer(text);
			return retval;
		}

		lb = cpos->p[lc];
		if(lc)
			lb_1 = cpos->p[lc-1];
		else {
			if( s > 0)
				s--;
			else
				s = text->lines.used - 1;
			
			lb_1 = text->lines.p[s].cpos->p[
					text->lines.p[s].cpos->used -1];
		}
		}
		break;
	case OL_SB_STR_REP:
		{
		Buffer const *buffer = text->lines.p[s].buffer;

		if(lc > (buffer->used - 1) ) {
			UnlockBuffer(text);
			return retval;
		}

		lb = lc;

		if(lc > 0)
			lb_1 = lc - 1;
		else {
			if( s > 0)
				s--;
			else
				s = text->lines.used - 1;
			
			lb_1 = text->lines.p[s].cpos->used - 1;
		}
		}
		break;
	case OL_WC_STR_REP:
		{
		Buffer const *buffer = text->lines.p[s].buffer;
		TextPosition nchars  = (buffer->used)/sizeof(wchar_t);

		if(lc > (nchars - 1) ) {
			UnlockBuffer(text);
			return retval;
		}

		lb = lc*sizeof(wchar_t);

		if(lc > 0)
			lb_1 = (lc-1)*sizeof(wchar_t);
		else {
			if( s > 0)
				s--;
			else
				s = text->lines.used - 1;
			
			lb_1 = (text->lines.p[s].cpos->used - 1)*sizeof(wchar_t);
		}
		}
		break;
} /* switch */

if (exp != NULL)
   {
   retval = SCAN_FOUND;
   (void) OlGetTextBufferStringAtLine(text, s, (TextLocation *)NULL);
   if ((c = Strrexp(text-> buffer-> p,
        &text-> buffer-> p[lb_1], exp)) == NULL)
      {
      for (s = location-> line - 1; s >= (TextLine)0; s--)
         {
         (void) OlGetTextBufferStringAtLine(text, s, (TextLocation *)NULL);
         if ((c = Strrexp(text-> buffer-> p, NULL, exp)) != NULL)
            break;
         }

      if (s < 0)
         {
         retval = SCAN_WRAPPED;
         for (s = text-> lines.used - 1; s > location-> line; s--)
            {
            (void) OlGetTextBufferStringAtLine(text, s, (TextLocation *)NULL);
            if ((c = Strrexp(text-> buffer-> p, NULL, exp)) != NULL)
               break;
            }

         if (s == location-> line &&
            (c == NULL || ((char *)c - (char *)text->buffer->p) <= lb))
            retval = SCAN_NOTFOUND;
         }
      }
   if (retval == SCAN_FOUND || retval == SCAN_WRAPPED)
      {
      location-> line   = s;
      location-> offset = OlGetCharOffset(c,text->buffer->p,text->strrep);
      location-> buffer = text-> buffer-> p;
      }
   }

UnlockBuffer(text);
return (retval);

} /* end of OlBackwardScanTextBuffer */

/*
 * OlNewPage
 *
 */

static TextPage
OlNewPage(OlTextBuffer *text, TextPage after)
{
TextPage at = after + 1;
register TextPage i = text-> pagecount;
register TextPage j;

#ifdef DEBUG_OLTEXTBUFF
   (void) fprintf(stderr,"allocating a new page at %d\n", at);
#endif

if (i == PQLIMIT)
   {
   for (i = 0, j = 1; j < PQLIMIT; j++)
      if (text-> pqueue[j].timestamp < text-> pqueue[i].timestamp)
         i = j;

   OlSwapOut(text, text-> pqueue[i].pageindex);
   }
else
   text-> pagecount++;

if (text-> pages.used == text-> pages.size)
   {
   text-> pages.size *= 2;
   text-> pages.p = (OlPage *) REALLOC
      ((char *)text-> pages.p, (unsigned)(text->pages.size*sizeof(OlPage)));
   }

if (at < text-> pages.used++)
   {
   for (j = OlFirstLineOfPage(text, at); j < text-> lines.used; j++)
      text-> lines.p[j].pageindex++;

   for (j = 0; j < text-> pagecount; j++)
      if (text-> pqueue[j].pageindex >= at)
         text-> pqueue[j].pageindex++;
   memmove((XtPointer)&text-> pages.p[at + 1], (XtPointer)&text-> pages.p[at],
           (text-> pages.used - at  - 1) * sizeof(OlPage));
   }

text-> pages.p[at].bytes = (TextPosition)0;
text-> pages.p[at].chars = (TextPosition)0;
text-> pages.p[at].lines = (TextLine)0;
text-> pages.p[at].qpos  = i;
text-> pages.p[at].dpos  = (BlockTable *)NULL;

text-> pqueue[i].pageindex = text-> curpageno = at;
text-> pqueue[i].timestamp = text-> pageref++;

#ifdef DEBUG_OLTEXTBUFF
Olprintbuffer(text);
(void) fprintf(stderr,"after adding a page at %d\n\n", at);
#endif

return (at);

} /* end of OlNewPage */

/*
 * OlSplitPage
 *
 */

/*
 * OlSwapIn
 *
 * The algorithm here is:
 *
 * 1. if the page is not in the queue:
 *    a. find the least recently used page and page it out
 *    b. set the qpos to this slot
 *    c. get the block used by this page and free them
 *    d. loop for each line starting at the first line for this page
 *       and allocate space and read in the line (freeing the blocks
 *       consumed).
 * 2. set the page queue location to the pageindex and increment the
 *    timestamp for the queue slot.
 */

static void
OlSwapIn(OlTextBuffer *text, TextPage pageindex) 
{
register TextPage  i = text-> pages.p[pageindex].qpos;
register TextPage  j;
register TextBlock block = 0;
register TextLine  n;
register BufferElement * p;

#ifdef DEBUG_OLTEXTBUFF
(void) fprintf(stderr, "swapping in %d\n", pageindex);
#endif

if (i == PQLIMIT)
   {
   if (text-> pagecount == PQLIMIT)
      {
      for (i = 0, j = 1; j < PQLIMIT; j++)
         if (text-> pqueue[j].timestamp < text-> pqueue[i].timestamp)
            i = j;
      OlSwapOut(text, text-> pqueue[i].pageindex);
      }
   else
      {
      i = text-> pagecount;
      text-> pagecount++;
      }

   text-> pages.p[pageindex].qpos = i;

   OlFreeBlock(text, pageindex, block++);
   for (j = OlFirstLineOfPage(text, pageindex);
        text-> lines.p[j].pageindex == pageindex && j < text-> lines.used; j++)
      {
      text-> lines.p[j].buffer-> size =
      n = text-> lines.p[j].buffer-> used;
      p = text-> lines.p[j].buffer-> p =
         (BufferElement *) malloc((unsigned)(n * sizeof(BufferElement)));
      while (n > text->textblockavailable)
         {
         (void) fread(p, 1, text->textblockavailable, text-> tempfile);
         n -= text->textblockavailable;
         p = &p[text->textblockavailable];
         OlFreeBlock(text, pageindex, block++);
         }
      if (n > 0)
         {
         (void) fread(p, 1, n, text-> tempfile);
         text->textblockavailable -= n;
         }
      }
   text-> pages.p[pageindex].dpos-> used = 0;
   }

text-> pqueue[i].pageindex = text-> curpageno = pageindex;
text-> pqueue[i].timestamp = text-> pageref++;

} /* end of OlSwapIn */

/*
 * OlSwapOut
 *
 * The algorithm here is:
 *
 * 1. if the tempfile is not open, open it and calculate a block size
 *    based on the history accumulated thus far. (NOTE: not implemented)
 * 2. if the dpos BlockTable has not been allocated, allocate one.
 * 3. Allocate a block to page out to.
 * 4. loop through the lines in the page (starting from the first line)
 *    and write out the lines filling the blocks (allocating new ones
 *    as necessary).
 * 5. Set the qpos (queue position) indicator to show that the page is
 *    not in the queue.
 */

static void
OlSwapOut(OlTextBuffer *text, TextPage pageindex)
{
register TextLine i;
register TextPosition n;
register BufferElement * p;

#ifdef DEBUG_OLTEXTBUFF
(void) fprintf(stderr, "swapping out %d\n", pageindex);
#endif

if (pageindex == -1)
   return;

if (text-> tempfile == NULL)
   {
   text-> blocksize = blocksize;
   text-> tempfile = tmpfile();
   }

if (text-> pages.p[pageindex].dpos == (BlockTable *)NULL)
   text-> pages.p[pageindex].dpos =
      (BlockTable *) AllocateBuffer(sizeof(TextBlock), btalloc);

OlAllocateBlock(text, pageindex);
for (i = OlFirstLineOfPage(text, pageindex);
     text-> lines.p[i].pageindex == pageindex && i < text-> lines.used; i++)
   {
   p = text-> lines.p[i].buffer-> p;
   n = text-> lines.p[i].buffer-> used;
   while (n > text->textblockavailable)
      {
      (void) fwrite(p, 1, text->textblockavailable, text-> tempfile);
      n -= text->textblockavailable;
      p = &p[text->textblockavailable];
      OlAllocateBlock(text, pageindex);
      }
   if (n > 0)
      {
      (void) fwrite(p, 1, n, text-> tempfile);
      text->textblockavailable -= n;
      }
   FREE(text-> lines.p[i].buffer-> p);
   text-> lines.p[i].buffer-> p = NULL;
   }

text-> pages.p[pageindex].qpos = PQLIMIT;

} /* end of OlSwapOut */

static TextLine
OlFirstLineOfPage(OlTextBuffer *text, TextPage page)
{
register TextPage i;
register TextLine l = 0;

for (i = 0; i < page; i++)
   l += text-> pages.p[i].lines;

return (l);

} /* end of OlFirstLineOfPage */

/*
 * OlAllocateBlock
 *
 */

static void
OlAllocateBlock(OlTextBuffer *text, TextPage pageindex)
{
TextBlock block;

if (BufferEmpty(text-> free_list))
   block = text-> blockcnt++;
else
   block = text-> free_list-> p[--text-> free_list-> used];

(void) fseek(text-> tempfile, (long)(block * text-> blocksize), seek_set);
text->textblockavailable = text-> blocksize;

if (BufferFilled(text-> pages.p[pageindex].dpos))
   GrowBuffer((Buffer *)text-> pages.p[pageindex].dpos, 66);

text-> pages.p[pageindex].dpos-> p[text-> pages.p[pageindex].dpos-> used++] =
   block;

} /* end of OlAllocateBlock */

/*
 * OlFreeBlock
 *
 */

static void OlFreeBlock(OlTextBuffer *text, 
			TextPage pageindex, 
			TextBlock blockindex)
{
TextBlock block = text-> pages.p[pageindex].dpos-> p[blockindex];

if (BufferFilled(text-> free_list))
   GrowBuffer((Buffer *)text-> free_list, text-> free_list-> size * 2);
text-> free_list-> p[text-> free_list-> used++] = block;

(void) fseek(text-> tempfile, (long)(block * text-> blocksize), seek_set);
text->textblockavailable = text-> blocksize;

} /* end of OlFreeBlock */


static EditResult
OlInsertBlockIntoTextBuffer(OlTextBuffer *text,
				TextLocation *location,
				OlStr	string)
{
OlBufferListPtr head, cur, temp;
int num_lines = 0;
EditResult retval = EDIT_SUCCESS;
TextPosition byte_offset = 0;
register int i = 0;

	if (string == (OlStr)NULL ||
		!(*str_methods[text->strrep].StrCmp)(string,
			(*str_methods[text->strrep].StrEmptyString)()))
   		return (EDIT_SUCCESS);

	if(location == 	(TextLocation *)NULL)
   		return(EDIT_FAILURE);

	switch(text->strrep) {
		case OL_MB_STR_REP:
			byte_offset = text->lines.p[location->line].
						cpos->p[location->offset];
			break;
		case OL_SB_STR_REP:
			byte_offset = location->offset;
			break;
		case OL_WC_STR_REP:
			byte_offset = location->offset *sizeof(wchar_t);
			break;
	}

	if (location->line >= (TextLine)0 && 
		location->line < text-> lines.used &&
    	  	location-> offset >= (TextLine)0 &&
	  	location->offset < text->lines.p[location->line].
							cpos->used) {
	  /* get the list of buffers from string */
  	  head = cur = OlGetBufferListFromString(text->strrep, string, 
							&num_lines);
	  if( (head == (OlBufferListPtr)NULL)  || !num_lines)
		return(EDIT_FAILURE);

     	  for(i =0; i < num_lines-1 && retval != EDIT_FAILURE; i++) {
      	  if (location->offset == 0) { /* insert at beginning of a line */
         	text-> insert.hint |= TEXT_BUFFER_INSERT_LINE;
         	retval = OlInsertLineIntoTextBuffer(text,location->line,
					cur->buffer, cur->cpos);
         	location-> line++;
         	location-> offset = 0;
       	  } else { 
		/* split the line and insert */
		text->insert.hint |= TEXT_BUFFER_INSERT_SPLIT_LINE;
		retval = OlInsertLineIntoTextBufferAtLoc(text,location,
					cur->buffer,cur->cpos);
          }
	 temp = cur;
	 cur = cur->next;
	 FREE((char *)temp);
         } /* for loop */

   	if (cur->cpos-> used > 1) { /* dangling last line */
      		text-> insert.hint |= TEXT_BUFFER_INSERT_CHARS;
      	retval = OlInsertCharsIntoTextBufferAtLoc(text,
					location,cur->buffer, cur->cpos);
   	}

   } /* end of if */  

return(retval);

} /* end of OlInsertBlockIntoTextBuffer */

/*
 * MLDeleteBlockInTextBuffer
 *
 */

static EditResult
OlDeleteBlockInTextBuffer(OlTextBuffer *text,
    			  TextLocation *startloc,
    			  TextLocation *endloc)
{
Buffer *work;
PositionTable *work2;
unsigned char *p;
int clen; /* length in chars */
int blen; /* length in bytes */
int null_len;
int i, j, k,l;
int startbyte, endbyte, basebyte;
EditResult retval = EDIT_SUCCESS;

PositionTable *startcpos =  text->lines.p[startloc->line].cpos;
PositionTable *endcpos =  text->lines.p[endloc->line].cpos;
Buffer *startbuffer =  text->lines.p[startloc->line].buffer;
Buffer *endbuffer =  text->lines.p[endloc->line].buffer;

/* pages */
OlPage *startpage = &text-> pages.p[text-> lines.p[endloc-> line].pageindex]; 
OlPage *endpage = &text-> pages.p[text-> lines.p[endloc-> line].pageindex]; 

switch(text->strrep) {
	case OL_MB_STR_REP:
		startbyte = startcpos->p[startloc->offset];
		endbyte = endcpos->p[endloc->offset];
		null_len = mbnull_len;
		break;
	case OL_SB_STR_REP:
		startbyte = startloc->offset;
		endbyte = endloc->offset;
		null_len = 1;
		break;
	case OL_WC_STR_REP:
		startbyte = startloc->offset*sizeof(wchar_t);
		endbyte = endloc->offset*sizeof(wchar_t);
		null_len = sizeof(wchar_t);
		break;
}
	

if (startloc-> line == endloc-> line) {
   clen = endloc-> offset - startloc-> offset;
   blen = endbyte - startbyte;
   if (clen > 0)
      if (clen == endcpos->used) 
	(void) fprintf(stderr,"delete a line!\n");
      else {
         text-> deleted.hint = TEXT_BUFFER_DELETE_SIMPLE;
         p = (unsigned char *)
		OlGetTextBufferStringAtLine(text, startloc-> line, 
						(TextLocation *)NULL);
         memmove((XtPointer)&p[startbyte],(XtPointer)&p[endbyte],
					endbuffer->used - endbyte);
	 if(startcpos->p != NULL && text->strrep == OL_MB_STR_REP) {
		basebyte = endcpos->p[endloc->offset] -
				startcpos->p[startloc->offset];
		for(l= 0; l< endcpos->used - endloc->offset; l++) 
			startcpos->p[l+startloc->offset] = 
				endcpos->p[l+endloc->offset] - basebyte;
	 }
         endbuffer-> used -= blen;
         endcpos-> used -= clen;
         endpage->bytes -=blen;
         endpage->chars -=clen;
      }
} else { /* startloc->line != endloc->line */
   	if (startloc-> offset == 0) {
      	i = startloc-> line;
      	text-> deleted.hint |= TEXT_BUFFER_DELETE_START_LINE;
  } else {
      text-> deleted.hint |= TEXT_BUFFER_DELETE_START_CHARS;
      p = (unsigned char *)OlGetTextBufferStringAtLine(text, startloc-> line, 
						(TextLocation *)NULL);
      blen = startbuffer-> used - startbyte;
      clen = startcpos-> used - startloc-> offset;
      switch(text->strrep) {
	case OL_MB_STR_REP:
      		p[startbyte] = mbnull;
		break;
	case OL_SB_STR_REP:
		p[startbyte] = '\0';
		break;
	case OL_WC_STR_REP:
		memmove((XtPointer)&p[startbyte], (XtPointer)&wcnull,
					sizeof(wchar_t));
		break;
      }

      startbuffer-> used -= (blen - null_len);
      startpage->bytes -= (blen - null_len);
      startcpos-> used -= (clen - 1);
      startpage->chars -= (clen - 1);
      i = startloc-> line + 1;
    }

   if (endbyte == endbuffer-> used) {
      k = endloc-> line;
      text-> deleted.hint |= TEXT_BUFFER_DELETE_END_LINE;
   } else {
      if (endloc-> offset != 0) {
         text-> deleted.hint |= TEXT_BUFFER_DELETE_END_CHARS;
         p = (unsigned char *)OlGetTextBufferStringAtLine(text, 
				endloc-> line, (TextLocation *)NULL);
         clen = endloc-> offset;
         blen = endbyte;
         endcpos-> used -= clen;
         endbuffer-> used -= blen;
         endpage->bytes-=blen;
         endpage->chars-=clen;
         memmove((XtPointer)p, (XtPointer)&p[blen], endbuffer-> used);
	 if(endcpos->p != NULL && text->strrep == OL_MB_STR_REP) {
		basebyte = endcpos->p[endloc->offset];
			for(l= 0; l < endcpos->used; l++) 
				endcpos->p[l] = 
					endcpos->p[l+endloc->offset] - 
								basebyte; 
	}
      }
      k = endloc-> line - 1;
    }

   for (j = i; j <= k && retval != EDIT_FAILURE; j++)
      retval = OlDeleteLineInTextBuffer(text, i);

   if (i != startloc-> line && k != endloc-> line && 
				retval != EDIT_FAILURE) {
      text-> deleted.hint |= TEXT_BUFFER_DELETE_JOIN_LINE;
      j = startloc-> line;
      (void) OlGetTextBufferStringAtLine(text, j, (TextLocation *)NULL);
      work = CopyBuffer(text-> buffer);
      work2 = (PositionTable *)CopyBuffer((Buffer *)text->cpos);
      OlDeleteLineInTextBuffer(text, j);
      (void) OlGetTextBufferStringAtLine(text, j, (TextLocation *)NULL);
      (void)OlInsertBytesIntoBuffer(text-> buffer, work,
					work->used - null_len, 0);
      if(text->cpos->p != NULL && text->strrep == OL_MB_STR_REP)
      	(void)OlInsertOffsetsIntoPosTab(text-> cpos,work2,0);
      else
		text->cpos->used += (work2->used - 1);

      text-> pages.p[text-> lines.p[j].pageindex].chars += 
						(work2-> used - 1);
      text-> pages.p[text-> lines.p[j].pageindex].bytes += 
					(work-> used - null_len);
      FreeBuffer(work);
      FreeBuffer((Buffer *)work2);
   }
 }
return (retval);
} /* end of DeleteBlockInTextBuffer */

/*
 * OlDeleteLineInTextBuffer
 *
 */

static EditResult
OlDeleteLineInTextBuffer(OlTextBuffer *text, TextLine at)
{
TextPage pageindex;
TextLine j;

if (at < 0 || at >= text-> lines.used)
   return (EDIT_FAILURE);
else
   {
   pageindex = text-> lines.p[at].pageindex;

   if (text-> lines.p[at].pageindex != text-> curpageno)
      OlSwapIn(text, pageindex);

   text-> pages.p[pageindex].bytes -= text-> lines.p[at].buffer-> used;
   text->pages.p[pageindex].chars -= text->lines.p[at].cpos->used;
   }
   if (--text-> pages.p[pageindex].lines == (TextLine)0)
      {
      int qpos = text-> pages.p[pageindex].qpos;
      text-> curpageno = -1;
      if (qpos != --text-> pagecount)
         {
         text-> pages.p[text-> pqueue[text-> pagecount].pageindex].qpos = qpos;
         text-> pqueue[qpos] = text-> pqueue[text-> pagecount];
         }
      if (pageindex != --text-> pages.used)
         {
#ifdef DEBUG_OLTEXTBUFF
         Olprintbuffer(text);
         fprintf(stderr, "free page\n");
#endif

         memmove(&text-> pages.p[pageindex],
                 &text-> pages.p[pageindex + 1],
                 (text-> pages.used - pageindex) * sizeof(OlPage));
         for (j = OlFirstLineOfPage(text, pageindex); j < text-> lines.used; j++)
            text-> lines.p[j].pageindex--;
         for (j = 0; j < text-> pagecount; j++)
            if (text-> pqueue[j].pageindex > pageindex)
               text-> pqueue[j].pageindex--;
#ifdef DEBUG_OLTEXTBUFF
         Olprintbuffer(text);
#endif
         }
      }

   FreeBuffer((Buffer *)text-> lines.p[at].buffer);

   if (at != --text-> lines.used)
      memmove(&text-> lines.p[at], &text-> lines.p[at + 1],
              (text-> lines.used - at) * sizeof(OlLine));

   return (EDIT_SUCCESS);

} /* OlDeleteLineInTextBuffer */

/*
 * OlReplaceCharInTextBuffer
 *
 * The \fIOlReplaceCharInTextBuffer\fR function is used to replace
 * the character in the OlTextBuffer \fItext\fR at \fIlocation\fR
 * with the character \fIc\fR.
 *
 * See also:
 *
 * OlReplaceBlockInTextBuffer(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern EditResult
OlReplaceCharInTextBuffer (OlTextBuffer *text, 
			   TextLocation *location, 
			   OlStr c, 
			   TextUpdateFunction f, 
			   XtPointer d)
{
EditResult retval;
TextLocation startloc = *location;
XtPointer ptr;

GetToken();
LockBuffer(text);
switch(text->strrep) {
	case OL_SB_STR_REP:
		{
		unsigned char *buffer;
		buffer = (unsigned char *)malloc(sizeof(unsigned char));

		buffer[0] = *(unsigned char *)c;
		buffer[1] = '\0';
		ptr = (XtPointer)buffer;
		}
		break;
	case OL_MB_STR_REP:
		{
		unsigned char *buffer;
		size_t nbytes;

		nbytes = mblen((XtPointer)c,MB_CUR_MAX); 
		buffer = (unsigned char *)malloc(nbytes+mbnull_len);
		memmove((XtPointer)buffer,(XtPointer)c,nbytes);
		buffer[nbytes] = mbnull;
		ptr = (XtPointer)buffer;
		}
		break;
	case OL_WC_STR_REP:
		{
		wchar_t *buffer;

		buffer = (wchar_t *)malloc((unsigned)sizeof(wchar_t)*2);
		buffer[0] = *((wchar_t *)c);
		buffer[1] = wcnull;
		ptr = (XtPointer)buffer;
		}
		break;
}

(void)OlNextLocation(text,location);
if (location->line == 0 && location->offset == 0)
	*location = startloc;
retval = OlReplaceBlockInTextBuffer(text, &startloc, 
				location, (OlStr)ptr, f, d);
UnlockBuffer(text);
ReleaseToken();
return retval;
} /* end of OlReplaceCharInTextBuffer */

/*
 * OlReplaceBlockInTextBuffer
 *
 * The \fIOlReplaceBlockInTextBuffer\fR function is used to update the contents
 * of the OlTextBuffer associated with \fItext\fR.  The characters stored
 * between \fIstartloc\fR and \fIendloc\fR are deleted and the \fIstring\fR
 * is inserted after \fIstartloc\fR.  If the edit succeeds the 
 * TextUpdateFunction \fIf\fR is called with the parameters: \fId\fR, 
 * \fItext\fR, and 1; then any other \fBdifferent\fR update functions
 * associated with the OlTextBuffer are called with their associated data
 * pointer, \fItext\fR, and 0.
 *
 * This function records the operation performed in OlTextUndoItem structures.
 * The contents of these structures can be used to implement an undo function.
 * The contents can also be used to determine the type of operation performed.
 * A structure is allocated for both the delete and insert information.
 *
 * The hints provided in these structures is the inclusive or of:~
 *
 *    TEXT_BUFFER_NOP
 *    TEXT_BUFFER_INSERT_LINE
 *    TEXT_BUFFER_INSERT_SPLIT_LINE
 *    TEXT_BUFFER_INSERT_CHARS
 *    TEXT_BUFFER_DELETE_START_LINE
 *    TEXT_BUFFER_DELETE_END_LINE
 *    TEXT_BUFFER_DELETE_START_CHARS
 *    TEXT_BUFFER_DELETE_END_CHARS
 *    TEXT_BUFFER_DELETE_JOIN_LINE
 *    TEXT_BUFFER_DELETE_SIMPLE
 *
 * See also:
 *
 * OlReplaceCharInTextBuffer(5)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern EditResult
OlReplaceBlockInTextBuffer(OlTextBuffer* text, TextLocation* startloc,
	TextLocation* endloc, OlStr string, TextUpdateFunction f, XtPointer d)
{
	int			i;
	EditResult		retval;
	TextLocation		startlocation = *startloc;
	TextLocation		endlocation = *endloc;

	LockBuffer(text);
	OlFreeTextUndoList(text);

	if (startloc->line != endloc->line ||
		startloc->offset != endloc->offset) {
	
		(void) OlPreviousLocation(text, endloc);

		text->deleted.string = OlGetTextBufferBlock(text, startloc,
			endloc);

		*endloc = endlocation;
	}

	if (string != (OlStr) NULL)
		switch (text->strrep) {

		case OL_SB_STR_REP:
			text->insert.string =
				strcpy(malloc(strlen(string) + 1), string);
			break;

		case OL_MB_STR_REP:
			text->insert.string =
				strcpy(malloc(strlen(string) + mbnull_len),
					string);
			break;

		case OL_WC_STR_REP:
			{
				wchar_t*			p;
				int			wclen = 0;

				p = (wchar_t*)string;

				while (*p != wcnull) {
					p++;
					wclen++;
				}

				wclen++;
				p = (wchar_t*)malloc(wclen* sizeof(wchar_t));
				memmove((XtPointer)p, string, wclen* sizeof(wchar_t));
				text->insert.string = (OlStr)p;
			}
			break;
		}

	text->insert.hint =
		text->deleted.hint = 0;

	text->deleted.start =
		text->insert.start =
		text->insert.end = startlocation;

	text->deleted.end = endlocation;

	text->deleted.start.buffer = text->insert.start.buffer =
		(char*)OlPositionOfLocation(text, startloc);

	text->deleted.end.buffer = (char*)OlPositionOfLocation(text, endloc);

	retval = OlDeleteBlockInTextBuffer(text, startloc, endloc);

	if (retval == EDIT_SUCCESS) {
		retval = OlInsertBlockIntoTextBuffer(text, startloc, string);

		if (retval == EDIT_SUCCESS) {

			text->dirty = TRUE;
			text->insert.end = *startloc;

			text->insert.end.buffer = 
				(char*)OlPositionOfLocation(text, startloc);

			for (i = 0; i < text->refcount; i++)
				if (text->update[i].f == f && 
					text->update[i].d == d) {

					(*text->update[i].f)(text->update[i].d,
						(XtPointer)text, 1);
					break;

				}

			for (i = 0; i < text->refcount; i++)
				if (text->update[i].f != f || 
					text->update[i].d != d)
					(*text->update[i].f)(text->update[i].d,
						(XtPointer)text, 0);
		}
	} else
		retval = EDIT_FAILURE;

	UnlockBuffer(text);
	return (retval);

} /* OlReplaceBlockInTextBuffer */


/*
 * OlLocationOfPosition
 *
 * The \fIOlLocationOfPosition\fR function is used to translate a 
 * \fIposition\fR in the \fItext\fR OlTextBuffer to a TextLocation.
 * It returns a pointer to the translated TextLocation in the third
 * argument. If the third argument is NULL, it allocates space
 * and returns a pointer to the allocated TextLocation.
 * If the \fIposition\fR is invalid
 * the Buffer pointer \fIbuffer\fP is set to NULL and the line and 
 * offset members are set the the last valid location in 
 * the OlTextBuffer; otherwise \fIbuffer\fP is set to a non-NULL 
 * (though useless) value.
 *
 * Note: The storage space for TextLocation if not provided by the 
 * caller is allocated by this function.
 *
 * See also:
 *
 * OlLineOfPosition(3), OlPositionOfLocation(3), OlLocationOfPosition(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern TextLocation
*OlLocationOfPosition(OlTextBuffer *text, 
			TextPosition position,
			TextLocation *location)
{
register TextPage i;
register TextPosition c = (TextPosition)0;
register TextLine l = (TextLine)0;

LockBuffer(text);
if(location == (TextLocation *)NULL)
	location = (TextLocation *)malloc(sizeof(TextLocation));

for (i = (TextPage)0; c < position && i < text->pages.used; i++)
   {
   c += text-> pages.p[i].chars;
   l += text-> pages.p[i].lines;
   }

if (position == (TextPosition)0)
   {
   location->line = (TextLine)0;
   location->offset = (TextPosition)0;
   if(text->strrep == OL_WC_STR_REP)
   	location->buffer = (char *)wc_empty_string;
   else
   	location->buffer = empty_string;
   }
else
   if (c <= position && i == text->pages.used)
      {
      location->line = l - 1;
      location->offset = c - 1;
      location->buffer = NULL;
      }
   else
      {
      i--;
      c -= text-> pages.p[i].chars;
      l -= text-> pages.p[i].lines;
      	while (c <= position)
         	c += text-> lines.p[l++].cpos-> used;
      	c -= text-> lines.p[--l].cpos-> used;
      	location->line   = l;
      	location->offset = position - c;
      	location->buffer = empty_string;
      }

UnlockBuffer(text);
return (location);

} /* end of OlLocationOfPosition */

/*
 * OlLineOfPosition
 *
 * The \fIOlLineOfPosition\fR function is used to translate a 
 * \fIposition\fR in the \fItext\fR OlTextBuffer to a line index.
 * It returns the translated line index or EOF if the \fIposition\fR
 * is invalid.
 *
 * See also:
 *
 * OlLineOfPositon(3), OlPositionOfLocation(3), OlLocationOfPosition(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern TextLine 
OlLineOfPosition(OlTextBuffer *text, TextPosition position)
{
TextLocation *location;

LockBuffer(text);
location = OlLocationOfPosition(text, position,NULL);
if (location->buffer == NULL)
   location->line = EOF;

UnlockBuffer(text);
return (location->line);

} /* end of OlLineOfPosition */

/*
 * OlPositionOfLine
 *
 * The \fIOlPositionOfLine\fR function is used to translate a 
 * \fIlineindex\fR in the \fItext\fR OlTextBuffer to a TextPosition.
 * It returns the translated TextPosition or EOF if the \fIlineindex\fR
 * is invalid.
 *
 * See also:
 *
 * OlLineOfPositon(3), OlPositionOfLocation(3), OlLocationOfPosition(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern TextPosition
OlPositionOfLine(OlTextBuffer *text, TextLine lineindex)
{
TextPage i;
TextLine j;
TextPosition position = (TextPosition)EOF;

LockBuffer(text);
if (lineindex >= (TextLine)0 && lineindex < text-> lines.used)
   {
   position = (TextPosition)0;
   for (i = (TextPage)0; i < text-> lines.p[lineindex].pageindex; i++)
      position += text-> pages.p[i].chars;
   for (j = OlFirstLineOfPage(text, i); j < lineindex; j++)
      	position += text-> lines.p[j].cpos-> used;
   }

UnlockBuffer(text);
return (position);

} /* end of OlPositionOfLine */

/*
 * OlLastTextBufferLocation
 *
 * The \fIOlLastTextBufferLocation\fR function returns the 
 * pointer to the last valid  
 * TextLocation in the OlTextBuffer associated with \fItext\fR.
 * If the third argument is NULL, space for last TextLocation is
 * allocated, otherwise the third arg contains the last valid
 * TextLocation.
 *
 * See also:
 *
 * OlLastTextBufferPosition(3), OlFirstTextBufferLocation(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern TextLocation 
*OlLastTextBufferLocation(OlTextBuffer *text, TextLocation *last)
{

LockBuffer(text);
if(last == NULL)
	last = (TextLocation *)malloc(sizeof(TextLocation));

last->line = text->lines.used - 1;
last->offset = text->lines.p[last->line].cpos->used - 1;
UnlockBuffer(text);

return (last);

} /* end of OlLastTextBufferLocation */

/*
 * OlLastTextBufferPosition
 *
 * The \fIOlLastTextBufferPosition\fR function returns the last valid
 * TextPositon in the OlTextBuffer associated with \fItext\fR.
 *
 * See also:
 *
 * OlLastTextBufferLocation(3), OlFirstTextBufferLocation(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern TextPosition
OlLastTextBufferPosition(OlTextBuffer *text)
{

return (OlPositionOfLocation(text, OlLastTextBufferLocation(text,NULL)));

} /* end of LastTextBufferPosition */

/*
 * OlPositionOfLocation
 *
 * The \fIOlPositionOfLocation\fR function is used to translate a 
 * \fIlocation\fR in the \fItext\fR OlTextBuffer to a TextPosition.
 * The function returns the translated TextPosition or EOF if 
 * the \fIlocation\fR is invalid.
 *
 * See also:
 *
 * OlPositionOfLine(3), OlLocationOfPosition(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern TextPosition
OlPositionOfLocation(OlTextBuffer *text, TextLocation *location)
{
TextPosition position; 

if(location == (TextLocation *)NULL)
	return((TextPosition)EOF);

LockBuffer(text);
position = OlPositionOfLine(text, location->line);

if (position != (TextPosition)EOF)
	if (location->offset >= 
			text->lines.p[location->line].cpos->used)
      			position = (TextPosition)EOF;
   	else
      			position += location->offset;
UnlockBuffer(text);
return (position);

} /* end of OlPostionOfLocation */

/*
 * OlGetTextBufferLine
 *
 * The \fIOlGetTextBufferLine\fR function is used to retrieve the 
 * contents of
 * \fIline\fR from the \fItext\fR OlTextBuffer.  It returns a pointer to a 
 * string containing the copy of the contents of the line or NULL if the 
 * \fIline\fR is outside the range of valid lines in \fItext\fR.
 *
 * Note:
 *
 * The storage for the copy is allocated by this routine.  It is the 
 * responsibility of the caller to free this storage when it becomes
 * dispensible.
 *
 * See also:
 *
 * OlGetTextBufferStringAtLine(3), OlOlGetTextBufferCharAtLoc(3), OlGetTextBufferBlock(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern OlStr 
OlGetTextBufferLine(OlTextBuffer *text, TextLine lineindex)
{
Buffer *buffer;

LockBuffer(text);
(void)OlGetTextBufferStringAtLine(text, lineindex, (TextLocation *)NULL);

if (text->buffer->p != NULL) {
	buffer = CopyBuffer(text->buffer);
	UnlockBuffer(text);
	return(buffer->p);
} else {
	UnlockBuffer(text);
	return((OlStr)NULL);
}


} /* end of MLGetTextBufferLine */

/*
 * OlGetTextBufferCharAtLoc
 *
 * The \flOlGetTextBufferCharAtLoc\fR function is used to retrieve a 
 * character
 * stored in the \fItext\fR OlTextBuffer at \fIlocation\fR.  It returns
 * either the character itself or EOF if location is outside the range
 * of valid locations within the OlTextBuffer.
 *
 * See also:
 *
 * OlGetTextBufferStringAtLine(3), OlGetTextBufferBlock(3), 
 * OlGetTextBufferLine(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern OlStr
OlGetTextBufferCharAtLoc(OlTextBuffer *text, TextLocation *location)
{

LockBuffer(text);
(void)OlGetTextBufferStringAtLine(text, 
				location->line, (TextLocation *)NULL);

if(text->buffer == NULL) {
	UnlockBuffer(text);
	return((OlStr)NULL);
}

switch(text->strrep) {
	case OL_SB_STR_REP:
		{
		if (text->buffer->p != NULL) 
   			if (location->offset < text->cpos->used) {
				unsigned char *c = (unsigned char *)
					malloc(sizeof(unsigned char));
      				*c = text->buffer->p[location->offset];
				*c = ( *c == '\0' ? '\n' : *c);
				UnlockBuffer(text);
				return((OlStr)c);
			}

		}
		UnlockBuffer(text);
		return((OlStr)NULL);
	case OL_WC_STR_REP:
		{
		wchar_t *p;

		if (text->buffer->p != NULL)
   		     if (location->offset< text->cpos->used) { 
			 wchar_t *c = (wchar_t *)malloc(sizeof(wchar_t));

			   p = (wchar_t *)text->buffer->p;
      			   *c =p[location->offset];
			   *c = (*c == (wchar_t)mbnull ?  
					(wchar_t)wcnewline : *c);
			   UnlockBuffer(text);
			   return((OlStr)c);
		     }

		}
		UnlockBuffer(text);
		return((OlStr)NULL);
	case OL_MB_STR_REP:
		{
		if (text->buffer->p != NULL && text->cpos->p != NULL)
   			if (location->offset< text->cpos->used) { 
				unsigned char *c;
				int len = 0;
				OlStr p; 

				p = (OlStr)&text->buffer->p
					[text->cpos->p[location->offset]];
				len = mblen(p,text->buffer->used -
					text->cpos->p[location->offset]);

				if(len == 0 && p != (OlStr)NULL) {
					UnlockBuffer(text);
					return((OlStr)strdup("\n"));
				}

				if(len == -1) {
				OlWarning(dgettext(OlMsgsDomain,
					"OlGetTextBufferCharAtLoc: Invalid multi-byte char\n"));
				UnlockBuffer(text);
				return((OlStr)NULL);
				}	
				
				c = (unsigned char *)malloc(
						sizeof(unsigned char)*len);
				memmove((XtPointer)c,(XtPointer)p,len);
				UnlockBuffer(text);
				return((OlStr)c);
			}

		}
		UnlockBuffer(text);
		return((OlStr)NULL);
 } /* end of switch */

} /* end of OlGetTextBufferCharAtLoc */

/*
 * OlCopyTextBufferBlock
 *
 * The \fIOlCopyTextBufferBlock\fR function is used to retrieve a 
 * text block
 * from the \fItext\fR OlTextBuffer.  
 * The block is defined as the characters
 * between \fIstart_position\fR and \fIend_position\fR inclusive.  
 *
 * Aarg2 should point to suffcient space; arg3
 * contains num_bytes arg2 points to; if num_bytes is not sufficient
 * OlCopyTextBufferBlock returns -1
 * else, function returns actual bytes used
 * Note:
 *
 * The storage for the copy is allocated by the caller.  It is the 
 * responsibility of the caller to ensure that enough storage is allocated
 * to copy end_position - start_position + bytes-to-store-null-character.
 *
 * See also:
 *
 * OlGetTextBufferStringAtLine(3), OlGetTextBufferCharAtLoc(3), 
 * OlGetTextBufferLine(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern int
OlCopyTextBufferBlock(OlTextBuffer *text,
			OlStr  outbuffer, 
			int    num_bytes,
			TextPosition start_position, 
			TextPosition end_position)
{
Boolean  retval = FALSE;
TextLocation *start_location;
TextLocation *end_location;
unsigned char *buffer = (unsigned char *)outbuffer;
int nchars; 

LockBuffer(text);
start_location = OlLocationOfPosition(text, start_position,
						(TextLocation *)NULL);
end_location =   OlLocationOfPosition(text, end_position, 
						(TextLocation *)NULL);

if (end_position < start_position ||
    start_location->buffer == NULL || 
	end_location->buffer == NULL) {
   	retval = 0;
} else {
   TextPosition size      = end_position - start_position + 1;
   TextLine     lineindex = start_location->line;
   TextPosition inpos     = start_location->offset;
   unsigned char *t = (unsigned char *)OlGetTextBufferStringAtLine(text, 
					lineindex, (TextLocation *)NULL);
   TextLine nlines = end_location->line - start_location->line;
   TextPosition startbyte , endbyte;
   int nbytes = 0;
   int lbytes = 0;
   register int i = 0;

   switch(text->strrep) {
	case OL_MB_STR_REP:
   num_bytes -= mbnull_len;

   if(nlines == 0) { /* extract chars from a single line */ 
	if(end_location->offset +1 >= text->cpos->used)
		endbyte = text->buffer->used;
	else
		endbyte = text->cpos->p[end_location->offset+1];	

	startbyte = text->cpos->p[start_location->offset];	
	nbytes = endbyte - startbyte;

	if(nbytes > num_bytes) {
		UnlockBuffer(text);
		return -1; 
	}
		
	memmove((XtPointer)buffer,(XtPointer)&t[startbyte],nbytes);
        if(endbyte == text->buffer->used) 
		buffer[nbytes - mbnewline_len] = mbnewline;
   } else if(nlines >= 1) { /* at least two lines */

			/* extract first line chars */
			endbyte = text->buffer->used;
			startbyte = text->cpos->p[start_location->offset];
			nbytes = endbyte - startbyte;
			if(nbytes > num_bytes) {
				UnlockBuffer(text);
				return -1;
			}
			memmove((XtPointer)buffer,
				(XtPointer)&t[startbyte], nbytes);
			buffer[nbytes - mbnewline_len] = mbnewline;

		/* extract complete lines */
	 	for(i=lineindex+1;i < end_location->line; i++) {
			lbytes = 0;

		   	t = (unsigned char *)OlGetTextBufferStringAtLine(
						text,i,(TextLocation *)NULL);

			lbytes = text->buffer->used;
			if(nbytes+lbytes  > num_bytes) {
				UnlockBuffer(text);
				return -1;
			}
			memmove((XtPointer)&buffer[nbytes],
					(XtPointer)t,lbytes);
			nbytes += lbytes;
			buffer[nbytes - mbnewline_len] = mbnewline;
		}

		/* extract chars from last line */
		   	t = (unsigned char *)OlGetTextBufferStringAtLine(
					text, i, (TextLocation *)NULL);
			if(end_location->offset +1 >= text->cpos->used)
				endbyte = text->buffer->used;
			else
				endbyte = 
				   text->cpos->p[end_location->offset+1];
			lbytes = endbyte;
			if(lbytes+nbytes > num_bytes) {
				UnlockBuffer(text);
				return -1;
			}
			memmove((XtPointer)&buffer[nbytes],(XtPointer)t, 
							lbytes);
			nbytes += lbytes;
        		if(endbyte == text->buffer->used) 
				buffer[nbytes - mbnewline_len] = mbnewline;
		}

	buffer[nbytes] = mbnull;
	nbytes += mbnull;
	retval = nbytes;
	break; 
	case OL_WC_STR_REP:
	if(num_bytes < (size+1)*sizeof(wchar_t)) {
		UnlockBuffer(text);
		return(-1);
	}
   if(nlines == 0) { /* extract chars from a single line */ 
	if(end_location->offset +1 >= text->cpos->used)
		endbyte = text->buffer->used;
	else
		endbyte = (end_location->offset+1)*sizeof(wchar_t);	

	startbyte = start_location->offset*sizeof(wchar_t);	
	nbytes = endbyte - startbyte;
	memmove((XtPointer)buffer,(XtPointer)&t[startbyte],nbytes);
        if(endbyte == text->buffer->used) 
		memmove((XtPointer)&buffer[nbytes - sizeof(wchar_t)],
					(XtPointer)&wcnewline, sizeof(wchar_t));
   } else if(nlines >= 1) { /* at least two lines */

			endbyte = text->buffer->used;
			startbyte = start_location->offset*sizeof(wchar_t);
			nbytes = endbyte - startbyte;
			memmove((XtPointer)buffer,
				(XtPointer)&t[startbyte], nbytes);
			memmove((XtPointer)&buffer[nbytes - sizeof(wchar_t)],
				(XtPointer)&wcnewline, sizeof(wchar_t));

		/* extract complete lines */
	 	for(i=lineindex+1;i < end_location->line; i++) {
			lbytes = 0;

		   	t = (unsigned char *)OlGetTextBufferStringAtLine(
					text, i, (TextLocation *)NULL);

			lbytes = text->buffer->used;
			memmove((XtPointer)&buffer[nbytes],
					(XtPointer)t,lbytes);
			nbytes += lbytes;
			memmove((XtPointer)&buffer[nbytes - sizeof(wchar_t)],
					(XtPointer)&wcnewline, sizeof(wchar_t));
		}

		/* extract chars from last line */
		   	t = (unsigned char *)OlGetTextBufferStringAtLine(
					text, i, (TextLocation *)NULL);
			if(end_location->offset +1 >= text->cpos->used)
				endbyte = text->buffer->used;
			else
				endbyte = (end_location->offset+1)*sizeof(wchar_t);
			lbytes = endbyte;
			memmove((XtPointer)&buffer[nbytes],(XtPointer)t, 
							lbytes);
			nbytes += lbytes;
        		if(endbyte == text->buffer->used) 
				memmove((XtPointer)
					&buffer[nbytes - sizeof(wchar_t)],
					(XtPointer)&wcnewline, sizeof(wchar_t));
	}

	memmove((XtPointer)&buffer[nbytes],&wcnull, sizeof(wchar_t));
	nbytes += sizeof(wchar_t);
	retval = nbytes;
	break; 
	case OL_SB_STR_REP:
	if(num_bytes < size +1) {
		UnlockBuffer(text);
		return(-1);
	}
   if(nlines == 0) { /* extract chars from a single line */ 
	if(end_location->offset +1 >= text->cpos->used)
		endbyte = text->buffer->used;
	else
		endbyte = end_location->offset+1;	

	startbyte = start_location->offset;	
	nbytes = endbyte - startbyte;
	memmove((XtPointer)buffer,(XtPointer)&t[startbyte],nbytes);
	if(endbyte == text->buffer->used) 
		buffer[nbytes-1] = '\n';
   } else if(nlines >= 1) { /* at least two lines */

			endbyte = text->buffer->used;
			startbyte = start_location->offset;
			nbytes = endbyte - startbyte;
			memmove((XtPointer)buffer,
				(XtPointer)&t[startbyte], nbytes);
			buffer[nbytes-1] = '\n';

		/* extract complete lines */
	 	for(i=lineindex+1;i < end_location->line; i++) {
			lbytes = 0;

		   	t = (unsigned char *)OlGetTextBufferStringAtLine(
					text, i, (TextLocation *)NULL);

			lbytes = text->buffer->used;
			memmove((XtPointer)&buffer[nbytes],
					(XtPointer)t,lbytes);
			nbytes += lbytes;
			buffer[nbytes-1] = '\n';
		}

		/* extract chars from last line */
		   	t = (unsigned char *)OlGetTextBufferStringAtLine(
				text, i, (TextLocation *)NULL);
			if(end_location->offset +1 >= text->cpos->used)
				endbyte = text->buffer->used;
			else
				endbyte = (end_location->offset+1);
			lbytes = endbyte;
			memmove((XtPointer)buffer[nbytes],(XtPointer)&t, 
							lbytes);
			nbytes += lbytes;
        		if(endbyte == text->buffer->used) 
				buffer[nbytes-1] = '\n';
	}

	buffer[nbytes++] = '\0';
	retval = nbytes;
	break; 
	} /* switch */


 } /* else */
UnlockBuffer(text);
return (retval);

} /* end of OlCopyTextBufferBlock */

/*
 * OlGetTextBufferBlock
 *
 * The \fIOlGetTextBufferBlock\fR function is used to retrieve a text block
 * from the \fItext\fR TextBuffer.  The block is defined as the characters
 * between \fIstart_location\fR and \fIend_location\fR inclusive.  It
 * returns a pointer to a string containing the copy.  If the parameters
 * are invalid NULL is returned.  
 *
 * Note:
 *
 * The storage for the copy is allocated by this routine.  It is the 
 * responsibility of the caller to free this storage when it becomes
 * dispensible.
 *
 * See also:
 *
 * OlGetTextBufferStringAtLine(3), OlGetTextBufferCharAtLoc(3), 
 * OlGetTextBufferLine(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern OlStr 
OlGetTextBufferBlock(OlTextBuffer *text, 
			TextLocation *start_location, 
			TextLocation *end_location)
{
Buffer *buffer = AllocateBuffer(sizeof(char), 0);
TextPosition start_position, end_position;
register int i = 0;

LockBuffer(text);
start_position = OlPositionOfLocation(text, start_location);
end_position   = OlPositionOfLocation(text, end_location);
if (start_position != (TextPosition)EOF && end_position != (TextPosition)EOF &&
    start_position <= end_position)
   {
   TextPosition size      = end_position - start_position + 1;
   TextLine     lineindex = start_location->line;
   TextPosition inpos     = start_location->offset;
   TextPosition outpos    = 0;
   char   *t = OlGetTextBufferStringAtLine(text, 
			lineindex,(TextLocation *)NULL);
   TextLine nlines = end_location->line - start_location->line;
   TextPosition startbyte , endbyte;
   int nbytes = 0;
   int lbytes = 0;

   switch(text->strrep) {
	case OL_MB_STR_REP:
   if(nlines == 0) { /* extract chars from a single line */ 
	if(end_location->offset +1 >= text->cpos->used)
		endbyte = text->buffer->used;
	else
		endbyte = text->cpos->p[end_location->offset+1];	

	startbyte = text->cpos->p[start_location->offset];	
	nbytes = endbyte - startbyte;
	GrowBuffer(buffer,nbytes);
	memmove((XtPointer)buffer->p,(XtPointer)&t[startbyte],nbytes);
        if(endbyte == text->buffer->used) 
		buffer->p[nbytes - mbnewline_len] = mbnewline;
   } else if(nlines >= 1) { /* at least two lines */

			endbyte = text->buffer->used;
			startbyte = text->cpos->p[start_location->offset];
			nbytes = endbyte - startbyte;
			GrowBuffer(buffer,nbytes);
			memmove((XtPointer)buffer->p,
				(XtPointer)&t[startbyte], nbytes);
			buffer->p[nbytes - mbnewline_len] = mbnewline;

		/* extract complete lines */
	 	for(i=lineindex+1;i < end_location->line; i++) {
			lbytes = 0;

		   	t = OlGetTextBufferStringAtLine(text, i,
                                                (TextLocation *)NULL);

			lbytes = text->buffer->used;
			GrowBuffer(buffer,lbytes);
			memmove((XtPointer)&buffer->p[nbytes],
					(XtPointer)t,lbytes);
			nbytes += lbytes;
			buffer->p[nbytes - mbnewline_len] = mbnewline;
		}

		/* extract chars from last line */
		   	t = OlGetTextBufferStringAtLine(text, i,
                                                (TextLocation *)NULL);
			if(end_location->offset +1 >= text->cpos->used)
				endbyte = text->buffer->used;
			else
				endbyte = 
				   text->cpos->p[end_location->offset+1];
			lbytes = endbyte;
			GrowBuffer(buffer,lbytes);
			memmove((XtPointer)&buffer->p[nbytes],(XtPointer)t, 
							lbytes);
			nbytes += lbytes;
        		if(endbyte == text->buffer->used) 
				buffer->p[nbytes - mbnewline_len] = 
							mbnewline;
		}

	GrowBuffer(buffer,mbnull_len);
	buffer->p[nbytes] = mbnull;
	nbytes += mbnull;
	buffer->used = nbytes;
	break; 
	case OL_WC_STR_REP:
   GrowBuffer(buffer,sizeof(wchar_t)*(size+1));

   if(nlines == 0) { /* extract chars from a single line */ 
	if(end_location->offset +1 >= text->cpos->used)
		endbyte = text->buffer->used;
	else
		endbyte = (end_location->offset+1)*sizeof(wchar_t);	

	startbyte = start_location->offset*sizeof(wchar_t);	
	nbytes = endbyte - startbyte;

	memmove((XtPointer)buffer->p,(XtPointer)&t[startbyte],nbytes);
        if(endbyte == text->buffer->used) 
		memmove((XtPointer)&buffer->p[nbytes - sizeof(wchar_t)],
					(XtPointer)&wcnewline, sizeof(wchar_t));
   } else if(nlines >= 1) { /* at least two lines */

			endbyte = text->buffer->used;
			startbyte = start_location->offset*sizeof(wchar_t);
			nbytes = endbyte - startbyte;
			memmove((XtPointer)buffer->p,
				(XtPointer)&t[startbyte], nbytes);
			memmove((XtPointer)&buffer->p[nbytes - sizeof(wchar_t)],
				(XtPointer)&wcnewline, sizeof(wchar_t));

		/* extract complete lines */
	 	for(i=lineindex+1;i < end_location->line; i++) {
			lbytes = 0;

		   	t = OlGetTextBufferStringAtLine(text, i,
                                                (TextLocation *)NULL);

			lbytes = text->buffer->used;
			memmove((XtPointer)&buffer->p[nbytes],
					(XtPointer)t,lbytes);
			nbytes += lbytes;
			memmove((XtPointer)&buffer->p[nbytes - sizeof(wchar_t)],
					(XtPointer)&wcnewline, sizeof(wchar_t));
		}

		/* extract chars from last line */
		   	t = OlGetTextBufferStringAtLine(text, i,
                                                (TextLocation *)NULL);
			if(end_location->offset +1 >= text->cpos->used)
				endbyte = text->buffer->used;
			else
				endbyte = (end_location->offset+1)*sizeof(wchar_t);
			lbytes = endbyte;
			memmove((XtPointer)&buffer->p[nbytes],(XtPointer)t, 
							lbytes);
			nbytes += lbytes;
        		if(endbyte == text->buffer->used) 
				memmove((XtPointer)
					&buffer->p[nbytes - sizeof(wchar_t)],
					(XtPointer)&wcnewline, sizeof(wchar_t));
	}

	memmove((XtPointer)&buffer->p[nbytes],&wcnull, sizeof(wchar_t));
	nbytes += sizeof(wchar_t);
	buffer->used = nbytes;
	break; 
	case OL_SB_STR_REP:
   GrowBuffer(buffer,size+1);
   if(nlines == 0) { /* extract chars from a single line */ 
	if(end_location->offset +1 >= text->cpos->used)
		endbyte = text->buffer->used;
	else
		endbyte = end_location->offset+1;	

	startbyte = start_location->offset;	
	nbytes = endbyte - startbyte;
	memmove((XtPointer)buffer->p,(XtPointer)&t[startbyte],nbytes);
	if(endbyte == text->buffer->used) 
		buffer->p[nbytes-1] = '\n';
   } else if(nlines >= 1) { /* at least two lines */

			endbyte = text->buffer->used;
			startbyte = start_location->offset;
			nbytes = endbyte - startbyte;
			memmove((XtPointer)buffer->p,
				(XtPointer)&t[startbyte], nbytes);
			buffer->p[nbytes-1] = '\n';

		/* extract complete lines */
	 	for(i=lineindex+1;i < end_location->line; i++) {
			lbytes = 0;

		   	t = OlGetTextBufferStringAtLine(text, i,
                                                (TextLocation *)NULL);

			lbytes = text->buffer->used;
			memmove((XtPointer)&buffer->p[nbytes],
					(XtPointer)t,lbytes);
			nbytes += lbytes;
			buffer->p[nbytes-1] = '\n';
		}

		/* extract chars from last line */
		   	t = OlGetTextBufferStringAtLine(text, i,
                                                (TextLocation *)NULL);
			if(end_location->offset +1 >= text->cpos->used)
				endbyte = text->buffer->used;
			else
				endbyte = (end_location->offset+1);
			lbytes = endbyte;
			memmove((XtPointer)&buffer->p[nbytes],
						(XtPointer)t,lbytes);
			nbytes += lbytes;
        		if(endbyte == text->buffer->used) 
				buffer->p[nbytes-1] = '\n';
	}

	buffer->p[nbytes++] = '\0';
	break; 
	} /* switch */

   }

   UnlockBuffer(text);
   return(buffer->p);
} /* end of OlGetTextBufferBlock */

/*
 * OlSaveTextBuffer
 *
 * The \fIOlSaveTextBuffer\fR function is used to write the contents
 * of the \fItext\fR OlTextBuffer to the file \fIfilename\fR.  It returns
 * a SaveResult which can be:~
 *
 * .so CWstart
 *    SAVE_FAILURE
 *    SAVE_SUCCESS
 * .so CWend
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern SaveResult
OlSaveTextBuffer(OlTextBuffer *text, char *filename)
{
FILE * fp;
register TextLine i;

/* NOTE: NEED TO CHECK IF FILE CAN BE WRITTEN, ETC */

LockBuffer(text);
if (filename == NULL)
   filename = text-> filename;
else
   {
   if (text-> filename != NULL)
      FREE(text-> filename);
   text-> filename = strcpy(malloc((unsigned)(strlen(filename) + 1)), filename);
   }

if (filename != NULL && (fp = fopen(filename, "w")) != NULL)
   {
   if (text->lines.used)
      for (i = (TextLine)0; i < text-> lines.used; i++) {
         (void)OlGetTextBufferStringAtLine(text, i,(TextLocation *)NULL);

	 switch(text->strrep) {
	 case OL_SB_STR_REP:
	 case OL_MB_STR_REP:
         	(void) fprintf(fp, "%s\n", text->buffer->p); 
		break;
	  case OL_WC_STR_REP:
		{
		size_t nbytes = sizeof(char)*text->cpos->used*MB_CUR_MAX;
		size_t n = 0;
		unsigned char *s = (unsigned char *)malloc(nbytes*
						sizeof(unsigned char));

		n =  wcstombs((char *)s, (wchar_t *)text->buffer->p, nbytes); 
		if(n == (size_t)-1) {
		  	OlWarning(dgettext(OlMsgsDomain,
			"OlSaveTextBuffer:wcstombs:a wide char code does \
not correspond to a valid multi byte code\n"));
   			  UnlockBuffer(text);
			  return(SAVE_FAILURE);
		} 
         	(void) fprintf(fp, "%s\n", s); 
		}
		break;
	} /* switch */
      } /* for */

   (void) fclose(fp);
   text-> dirty = FALSE;
   UnlockBuffer(text);
   return (SAVE_SUCCESS);
   }
else {
   UnlockBuffer(text);
   return (SAVE_FAILURE);
}

} /* end of OlSaveTextBuffer */

/*
 * OlGetTextBufferBuffer
 *
 * The \fIOlGetTextBufferBuffer\fR function is used to retrieve a 
 * pointer to the
 * Buffer stored in OlTextBuffer \fItext\fR for \fIline\fR.  This pointer
 * is volatile; subsequent calls to any OlTextBuffer routine may make it
 * invalid.  If a more permanent copy of this Buffer is required the Buffer
 * Utility CopyBuffer can be used to create a private copy of it.
 *
 * See also:
 *
 * OlGetTextBufferBlock(3), OlGetTextBufferStringAtLine(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern Buffer *
OlGetTextBufferBuffer(OlTextBuffer *text, TextLine line)
{
OlStr p;
Buffer *retval;

LockBuffer(text);
p = OlGetTextBufferStringAtLine(text, line, (TextLocation *)NULL);

retval = text->buffer;
UnlockBuffer(text);
return (p  == NULL ? (Buffer *)NULL : retval);

} /* end of OlGetTextBufferBuffer */

/*
 * OlIncrementTextBufferLocation
 *
 * The \fIIncrementTextBufferLocation\fR function is used to increment
 * a \fIlocation\fR by a either \fIline\fR lines and/or \fIoffset\fR characters.
 * It returns the new location.  Note: if \fIline\fR or \fIoffset\fR are
 * negative the function performs a decrement operation.  If the starting
 * location or the resulting location is invalid the starting location is 
 * returned without modification; otherwise the new location is returned.
 *
 * See also:
 *
 * OlNextLocation(3), OlPreviousLocation(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern TextLocation
*OlIncrementTextBufferLocation(OlTextBuffer *text, 
				TextLocation *location, 
				TextLine line, 
				TextPosition offset)
{
TextLocation *newlocation = NULL;

if(location == NULL)
	return((TextLocation *)NULL);

LockBuffer(text);

if (location->line >= 0 			&& 
	location->line < text-> lines.used 	&&
   	location->offset >= 0 			&&
   	location->offset < text-> lines.p[location->line].cpos-> used) {
   if (line == 0 && offset == 0)
   	newlocation = location;
   else
      if (line == 0 && offset != 0) {
         TextPosition new_pos = 
		OlPositionOfLocation(text, location) + offset;

         if (new_pos >= 0) {
		newlocation = OlLocationOfPosition(text, new_pos, 
						(TextLocation *)NULL);
		if (newlocation->buffer == NULL) {
			if(newlocation && (newlocation != location)) {
	       			XtFree((XtPointer)newlocation);
				newlocation = NULL;
			}
               		newlocation = location;
            	}
         } else
            newlocation = location;
      } else {
	 	newlocation = (TextLocation *)XtMalloc(sizeof(TextLocation));
         	newlocation->line = location->line + line;
         	if (newlocation->line >= 0 && 
			newlocation->line < text-> lines.used) {
            		newlocation->offset = offset == 0 ?
               				location->offset : offset;
            		if (newlocation->offset >=
                		text-> lines.p[newlocation->line].cpos-> used)
               			newlocation->offset =
                  			text-> lines.
					p[newlocation->line].cpos-> used - 1;
            	} else {
			if(newlocation && (newlocation != location)) {
	    			XtFree((XtPointer)newlocation);
				newlocation = NULL;
			}
            		newlocation = location;
	 	}
      }
   } else
   	newlocation = location;

   *location = *newlocation;
   if(newlocation && (newlocation != location))
	XtFree((XtPointer)newlocation);

UnlockBuffer(text);
return (location);

} /* end of OlIncrementTextBufferLocation */

/*
 * OlPreviousLocation
 *
 * The \fIOlPreviousLocation\fR function returns the Location which 
 * precedes
 * the given \fIcurrent\fR location in a OlTextBuffer.  If the current
 * location points to the beginning of the OlTextBuffer this function wraps *  .
 * Note: the second argument is modified.
 *
 * See also:
 *
 * OlNextLocation(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern TextLocation
*OlPreviousLocation(OlTextBuffer *text, 
			TextLocation *current)
{
if(current == (TextLocation *)NULL)
	return((TextLocation *)NULL);

LockBuffer(text);
if (--current->offset < 0)
   {
   if (--current->line < 0)
      current->line = text->lines.used - 1;
      current->offset =  text->lines.p[current->line].cpos->used - 1;
   }

UnlockBuffer(text);
return (current);

} /* end of PreviousLocation */

/*
 * OlNextLocation
 *
 * The \fIOlNextLocation\fR function returns the TextLocation which follows
 * the given \fIcurrent\fR location in a OlTextBuffer.  If the current
 * location points to the end of the OlTextBuffer this function wraps.
 *
 * Note: the location passed to this function is modified.
 *
 * See also:
 *
 * OlPreviousLocation(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern TextLocation
*OlNextLocation(OlTextBuffer *text, 
		TextLocation *current)
{

LockBuffer(text);
if (++current->offset > text->lines.p[current->line].cpos->used -1) 
   {
   if (++current->line > text->lines.used - 1)
      		current->line = 0;
   current->offset = 0;
   }

UnlockBuffer(text);
return (current);

} /* end of OlNextLocation */

/*
 * _OlNextLocationWithoutWrap
 *
 * This function returns the TextLocation which follows
 * the given current location in a OlTextBuffer.
 */

extern TextLocation
*_OlNextLocationWithoutWrap(OlTextBuffer *text,
        TextLocation *current)
{
if (++current->offset > text->lines.p[current->line].cpos->used -1)
   {
   if (++current->line > text->lines.used - 1)
            current->line = text->lines.used - 1;
   current->offset = text->lines.p[current->line].cpos->used -1;
   }
return (current);
}

/*
 * OlStartCurrentTextBufferWord
 *
 * The \fIOlStartCurrentTextBufferWord\fR function is used to locate the 
 * beginning
 * of a word in the OlTextBuffer relative to a given \fIcurrent\fR 
 * location.
 * The function returns the location of the beginning of the current
 * word.  Note: this return value will equal the given current value
 * if the current location is the beginning of a word.
 *
 * Note: if the location is not in a word, it returns the start of the
 * "not word" region it is in.
 * 
 * See also:
 *
 *
 * MPreviousTextBufferWord(3), OlNextTextBufferWord(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern TextLocation
*OlStartCurrentTextBufferWord(OlTextBuffer *text, 
				TextLocation *current)
{
OlStrWordDefFunc isawordc;

LockBuffer(text);
isawordc = text->word_def_func;
(void)OlGetTextBufferStringAtLine(text, current->line, NULL);

switch(text->strrep) {
case OL_SB_STR_REP:
if ((*isawordc)(&text->buffer->p[current->offset]))
   {
   while(current->offset >= 0 
	&& (*isawordc)(&text->buffer->p[current->offset]))
      current->offset--;
   }
else
   {
   while(current->offset >= 0 && 
   	!(*isawordc)(&text->buffer->p[current->offset]))
      current->offset--;
   }
break;
case OL_WC_STR_REP:
if ((*isawordc)(&text->buffer->p[current->offset*sizeof(wchar_t)]))
   {
   while(current->offset >= 0 
	&& (*isawordc)(&text->buffer->p[current->offset*sizeof(wchar_t)]))
      current->offset--;
   }
else
   {
   while(current->offset >= 0 && 
   	!(*isawordc)(&text->buffer->p[current->offset*sizeof(wchar_t)]))
      current->offset--;
   }
break;
case OL_MB_STR_REP:
if ((*isawordc)(&text->buffer->p[text->cpos->p[current->offset]]))
   {
   while(current->offset >= 0 
	&& (*isawordc)(&text->buffer->p[text->cpos->p[current->offset]]))
      current->offset--;
   }
else
   {
   while(current->offset >= 0 && 
   	!(*isawordc)(&text->buffer->p[text->cpos->p[current->offset]]))
      current->offset--;
   }
break;
} /* switch */

current->offset++;

UnlockBuffer(text);
return (current);
} /* end of OlStartCurrentTextBufferWord */

/*
 * OlEndCurrentTextBufferWord
 *
 * The \fIOlEndCurrentTextBufferWord\fR function is used to locate the end
 * of a word in the OlTextBuffer relative to a given 
 * \fIcurrent\fR location.
 * The function returns the location of the end of the current
 * word.  Note: this return value will equal the given currrent value
 * if the current location is already at the end of a word.
 *
 * Note: if the region is not in a word, it returns the end of the
 * "not word" region it is in.
 *
 * See also:
 *
 * OlPreviousTextBufferWord(3), OlNextTextBufferWord(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern TextLocation
*OlEndCurrentTextBufferWord(OlTextBuffer *text, 
			    TextLocation *current)
{
OlStrWordDefFunc isawordc;
int end;

LockBuffer(text);
isawordc = text->word_def_func;
end = text->lines.p[current->line].cpos->used - 1;
(void)OlGetTextBufferStringAtLine(text, current->line, NULL);

switch(text->strrep) {
case OL_SB_STR_REP:
if ((*isawordc)(&text->buffer->p[current->offset]))
   {
   while(current->offset < end && 
	(*isawordc)(&text->buffer->p[current->offset]))
      current->offset++;
   }
else
   {
   while(current->offset < end && 
	!(*isawordc)(&text->buffer->p[current->offset]))
      current->offset++;
   }
break;
case OL_WC_STR_REP:
if ((*isawordc)(&text->buffer->p[sizeof(wchar_t)*current->offset]))
   {
   while(current->offset < end && 
	(*isawordc)(&text->buffer->p[sizeof(wchar_t)*current->offset]))
      current->offset++;
   }
else
   {
   while(current->offset < end && 
	!(*isawordc)(&text->buffer->p[sizeof(wchar_t)*current->offset]))
      current->offset++;
   }
break;
case OL_MB_STR_REP:
if ((*isawordc)(&text->buffer->p[text->cpos->p[current->offset]]))
   {
   while(current->offset < end && 
	(*isawordc)(&text->buffer->p[text->cpos->p[current->offset]]))
      		current->offset++;
   }
else
   {
   while(current->offset < end && 
	!(*isawordc)(&text->buffer->p[text->cpos->p[current->offset]]))
      		current->offset++;
   }
break;
}

UnlockBuffer(text);
return (current);

} /* end of OlEndCurrentTextBufferWord */

/*
 * OlPreviousTextBufferWord
 *
 * The \fIOlPreviousTextBufferWord\fR function is used to locate the
 * beginning of a word in a OlTextBuffer relative to a given \fIcurrent\fR
 * location.  It returns the location of the beginning of the word 
 * which precedes the given current location.  If the current location
 * is within a word this function will skip over the current word.
 * If the current word is the first word in the OlTextBuffer the function
 * wraps to the end of the buffer.
 *
 * See also:
 *
 * OlPreviousTextBufferWord(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern TextLocation
*OlPreviousTextBufferWord(OlTextBuffer *text, 
			  TextLocation *current)
{
OlStrWordDefFunc isawordc;
int byte_offset;
TextLine save_line;
TextLocation *retval;

LockBuffer(text);
isawordc = text->word_def_func;
(void)OlGetTextBufferStringAtLine(text, current->line, 
						(TextLocation *)NULL);
switch(text->strrep) {
	case OL_SB_STR_REP:
		byte_offset = current->offset;
		break;
	case OL_MB_STR_REP:
		byte_offset = text->cpos->p[current->offset];
		break;
	case OL_WC_STR_REP:
		byte_offset = current->offset*sizeof(wchar_t);
		break;
}

if(!(*isawordc)(&text->buffer->p[byte_offset])) {
	(void)OlStartCurrentTextBufferWord(text,current);
	save_line = current->line;
	(void)OlPreviousLocation(text,current);
	if(save_line != current->line)  {
		retval = OlPreviousTextBufferWord(text,current);
		UnlockBuffer(text);
		return retval;
	}
	(void)OlStartCurrentTextBufferWord(text,current);
} else {
	(void)OlStartCurrentTextBufferWord(text,current);
	save_line = current->line;
	(void)OlPreviousLocation(text,current);
	if(save_line != current->line) {
		retval = OlPreviousTextBufferWord(text,current);
		UnlockBuffer(text);
		return retval;
	}
	(void)OlStartCurrentTextBufferWord(text,current);
	save_line = current->line;
	(void)OlPreviousLocation(text,current);
	if(save_line != current->line) {
		retval =  OlPreviousTextBufferWord(text,current);
		UnlockBuffer(text);
                return retval; 
        }
	(void)OlStartCurrentTextBufferWord(text,current);
	}
UnlockBuffer(text);
return(current);

} /* end of OlPreviousTextBufferWord */
/*
 * OlNextTextBufferWord
 *
 * The \fIOlNextTextBufferWord\fR function is used to locate the beginning
 * of the next word from a given \fIcurrent\fR location in a OlTextBuffer.
 * If the current location is within the last word in the OlTextBuffer
 * the function wraps to the beginning of the OlTextBuffer.
 *
 * See also:
 *
 * OlPreviousTextBufferWord(3), OlStartCurrentTextBufferWord(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern TextLocation
*OlNextTextBufferWord (OlTextBuffer *text, 
			TextLocation *current)
{
OlStrWordDefFunc isawordc;
int byte_offset;
TextLine save_line;
TextLocation *retval;

LockBuffer(text);
isawordc = text->word_def_func;
(void)OlGetTextBufferStringAtLine(text, current->line, 
						(TextLocation *)NULL);
switch(text->strrep) {
	case OL_SB_STR_REP:
		byte_offset = current->offset;
		break;
	case OL_MB_STR_REP:
		byte_offset = text->cpos->p[current->offset];
		break;
	case OL_WC_STR_REP:
		byte_offset = current->offset*sizeof(wchar_t);
		break;
}

if(!(*isawordc)(&text->buffer->p[byte_offset])) {
			/* not in a word */
	(void)OlEndCurrentTextBufferWord(text,current);
	if(current->offset  == 
			OlLastCharInTextBufferLine(text,current->line)) {
	(void)OlNextLocation(text,current);
	(void)OlGetTextBufferStringAtLine(text, current->line, 
					(TextLocation *)NULL);
	if(!(*isawordc)(text->buffer->p) ) {
		retval = OlNextTextBufferWord(text,current);
		UnlockBuffer(text);
		return retval;
	}
	}
} else { 
		/* in a word */
	(void)OlEndCurrentTextBufferWord(text,current);
	(void)OlEndCurrentTextBufferWord(text,current);
	if(current->offset  == 
			OlLastCharInTextBufferLine(text,current->line)) {
	(void)OlNextLocation(text,current);
	(void)OlGetTextBufferStringAtLine(text, current->line, 
					(TextLocation *)NULL);
	if(!(*isawordc)(text->buffer->p) ) {
		retval = OlNextTextBufferWord(text,current);
		UnlockBuffer(text);
                return retval; 
        }
	}

	}

UnlockBuffer(text);
return(current);

} /* end of OlNextTextBufferWord */
/*
 * OlRegisterTextBufferUpdate
 *
 * The \fIOlRegisterTextBufferUpdate\fR procedure associates the 
 * TextUpdateFunction \fIf\fR and data pointer \fId\fR with the
 * given OlTextBuffer \fItext\fR.  
 * This update function will be called whenever an update operation
 * is performed on the OlTextBuffer.  See OlReplaceBlockInTextBuffer
 * for more details.
 *
 * Note:
 *
 * Calling this function increments a reference count mechanism 
 * used to determine when to actually free the OlTextBuffer.  Calling 
 * the function with a NULL value for the function circumvents 
 * this mechanism.
 *
 * See also:
 * 
 * OlUnregisterTextBufferUpdate(3), OlReadStringIntoTextBuffer(3),
 * OlReadFileIntoTextBuffer(3)
 *
 * Synopsis:
 *
 * #include<Oltextbuff.h>
 *  ...
 */

extern void
OlRegisterTextBufferUpdate(OlTextBuffer *text, 
				TextUpdateFunction f,
				XtPointer d)
{
register int i;

LockBuffer(text);
if (f != NULL)
   {
   i = text-> refcount++;

   text-> update = (TextUpdateCallback *)
      REALLOC((char *)text-> update,
	      text-> refcount * sizeof(TextUpdateCallback));

   text-> update[i].f = f;
   text-> update[i].d = d;
   }

UnlockBuffer(text);
} /* end of OlRegisterTextBufferUpdate */

/*
 * OlUnregisterTextBufferUpdate
 *
 * The \fIOlUnregisterTextBufferUpdate\fR function disassociates the 
 * TextUpdateFunction \fIf\fR and data pointer \fId\fR with the
 * given OlTextBuffer \fItext\fR.  If the function/data pointer pair
 * is not associated with the given OlTextBuffer zero is returned
 * otherwise the association is dissolved and one is returned.
 *
 * See also:
 * 
 * OlRegisterTextBufferUpdate(3), OlFreeTextBuffer(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern int
OlUnregisterTextBufferUpdate(OlTextBuffer *text,
				TextUpdateFunction f, 
				XtPointer d)
{
register int i;
int retval;

LockBuffer(text);
for (i = 0; i < text-> refcount; i++)
   if (text-> update[i].f == f && text-> update[i].d == d)
      break;

if (i == text-> refcount)
   retval = 0;
else
   {
   text-> refcount--;
   if (text-> refcount == 0)
      {
      FREE((char *)text-> update);
      text-> update = (TextUpdateCallback *)NULL;
      }
   else
      {
      if (i < text-> refcount)
         memmove(&text-> update[i],
                 &text-> update[i+1],
                 (text-> refcount - i) * sizeof(TextUpdateCallback));
      text-> update = (TextUpdateCallback *)
	  REALLOC((char *)text-> update,
		  text-> refcount * sizeof(TextUpdateCallback));
      }
   retval = 1;
   }

UnlockBuffer(text);
return (retval);

} /* end of OlUnregisterTextBufferUpdate */

/*
 * OlInitTextUndoList
 *
 */

static void
OlInitTextUndoList(OlTextBuffer *text)
{

text-> insert.string =
text-> deleted.string = NULL;

} /* end of OlInitTextUndoList */

/*
 * OlFreeTextUndoList
 *
 */

static void
OlFreeTextUndoList(OlTextBuffer *text)
{

if (text-> insert.string != NULL)
   FREE(text-> insert.string);

if (text-> deleted.string != NULL)
   FREE(text-> deleted.string);

OlInitTextUndoList(text);

} /* end of OlFreeTextUndoList */

/*
 * OlRegisterAllTextBufferScanFunctions
 *
 * The \fIOlRegisterTextBufferScanFunctions\fR procedure provides the 
 * capability
 * to replace the scan functions used by the OlForwardScanTextBuffer and
 * OlBackwardScanTextBuffer functions.  These functions are called as:~
 *
 * .so CWstart
 * 	(*forward)(string, curp, expression);
 * 	(*backward)(string, curp, expression);
 * .so CWend
 *
 * and are responsible for returning either a pointer to the begining
 * of a match for the expression or NULL.
 *
 * Calling this procedure with NULL function pointers reinstates the
 * default regular expression facility.
 * 
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern void
OlRegisterAllTextBufferScanFunctions(OlStrRep strrep,
					OlStrScanDefFunc forward,
					OlStrScanDefFunc backward)
{
switch(strrep) {
	case OL_SB_STR_REP:
		sbfwd = (forward ? forward : SB_fwd_scan_func);
		sbbwd = (backward ? backward : SB_bwd_scan_func);
		break;
	case OL_MB_STR_REP:
		mbfwd = (forward ? forward : MB_fwd_scan_func);
		mbbwd = (backward ? backward : MB_bwd_scan_func);
		break;
	case OL_WC_STR_REP:
		wcfwd = (forward ? forward : WC_fwd_scan_func);
		wcbwd = (backward ? backward : WC_bwd_scan_func);
		break;
}

} /* end of OlRegisterallTextBufferScanFunctions */

extern void
OlRegisterPerTextBufferScanFunctions(OlTextBuffer  *text,
					OlStrScanDefFunc forward,
					OlStrScanDefFunc backward)
{
LockBuffer(text);
switch(text->strrep) {
	case OL_SB_STR_REP:
		text->fwd_scan_func = 
				(forward ? forward : SB_fwd_scan_func);
		text->bwd_scan_func = 
			(backward ? backward : SB_bwd_scan_func);
		break;
	case OL_MB_STR_REP:
		text->fwd_scan_func = 
				(forward ? forward : MB_fwd_scan_func);
		text->bwd_scan_func = 
			(backward ? backward : MB_bwd_scan_func);
		break;
	case OL_WC_STR_REP:
		text->fwd_scan_func = 
				(forward ? forward : WC_fwd_scan_func);
		text->bwd_scan_func = 
			(backward ? backward : WC_bwd_scan_func);
		break;
}
UnlockBuffer(text);

} /* end of OlRegisterPreTextBufferScanFunctions */

/*
 * OlRegisterAllTextBufferWordDefinition
 *
 * The \fIOlRegisterAllTextBufferWordDefinition\fR procedure provides the 
 * capability to replace the word definition function used by the 
 * OlTextBuffer Utilities.  This function is called as:~
 *
 * .so CWstart
 * 	(*word_definition)(c);
 * .so CWend
 *
 * The function is responsible for returning TRUE if the character
 * c is considered a character that can occur in a word and FALSE 
 * otherwise.
 *
 * Calling this function with NULL reinstates the default word definition
 * which allows the following set of characters: a-zA-Z0-9_
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 *  ...
 */

extern void
OlRegisterAllTextBufferWordDefinition(OlStrRep strrep,
				 OlStrWordDefFunc word_definition)
{

switch(strrep) {
	case OL_SB_STR_REP:
		issbwrdc = (word_definition ? word_definition : 
						is_a_SB_word_char);
		break;
	case OL_MB_STR_REP:
		ismbwrdc = (word_definition ? word_definition : 
						is_a_MB_word_char);
		break;
	case OL_WC_STR_REP:
		iswcwrdc = (word_definition ? word_definition : 
						is_a_WC_word_char);
		break;
}

} /* end of OlRegisterAllTextBufferWordDefinition */

extern void
OlRegisterPerTextBufferWordDefinition(OlTextBuffer *text,
				 OlStrWordDefFunc word_definition)
{

LockBuffer(text);
switch(text->strrep) {
	case OL_SB_STR_REP:
		text->word_def_func = (word_definition ? word_definition : 
						is_a_SB_word_char);
		break;
	case OL_MB_STR_REP:
		text->word_def_func = (word_definition ? word_definition : 
						is_a_MB_word_char);
		break;
	case OL_WC_STR_REP:
		text->word_def_func = (word_definition ? word_definition : 
						is_a_WC_word_char);
		break;
}

UnlockBuffer(text);
} /* end of OlRegisterPerTextBufferWordDefinition */

/*
 * is_a_SB_word_char
 *
 */

static Boolean 
is_a_SB_word_char(OlStr s)
{
unsigned char  c = (unsigned char)(*((unsigned char *)s));
Boolean retval = isalnum((int)c) ||
		 (c == '_')	 ||
		 (192 <= (int)c  && (int)c <=255); 

return (retval);

} /* end of is_a_SB_word_char */

static Boolean 
is_a_MB_word_char(OlStr s)
{
Boolean retval= FALSE; 
int size;

if((size = mblen((char *)s, MB_CUR_MAX)) == 1)
	retval = (isgraph((int)(*((unsigned char *)s))) == 0 ? FALSE : TRUE);  
else if(size > 1)
	retval = TRUE;
	
return (retval);

} /* end of is_a_MB_word_char */

static Boolean 
is_a_WC_word_char(OlStr s)
{
wchar_t wc = *(wchar_t *)s;
unsigned char *c;
Boolean retval = FALSE;

retval = (iswgraph(wc) == 0 ? FALSE : TRUE);
return(retval);
} /* end of is_a_WC_word_char */

static EditResult 
OlInsertCharsIntoTextBufferAtLoc(OlTextBuffer *text, 
					TextLocation *location,
		 			Buffer *sbuffer, 
		 			PositionTable *scpos)
{
TextLine at; 
TextPosition offset;
TextPosition byte_offset;
Buffer *buffer;
PositionTable *cpos;
OlPage *page;
int null_len = 0;

if(location == (TextLocation *)NULL)
   return (EDIT_FAILURE);
		
at = location->line;
page = &text->pages.p[text->lines.p[at].pageindex];
offset = location->offset;


   (void)OlGetTextBufferStringAtLine(text,at,(TextLocation *)NULL);
   buffer = text->buffer;
   cpos = text->cpos;

   switch(text->strrep) {
	case OL_MB_STR_REP:
		byte_offset = cpos->p[offset];
		null_len = mbnull_len;
		break;
	case OL_SB_STR_REP:
		byte_offset = offset;
		null_len = 1;
		break;
	case OL_WC_STR_REP:
		byte_offset = offset*sizeof(wchar_t);
		null_len = sizeof(wchar_t);
		break;
    }
		/* insert new char buffers */
		if(!OlInsertBytesIntoBuffer(buffer,sbuffer,
			sbuffer->used - null_len,byte_offset))
			return(EDIT_FAILURE);
		if(scpos->p != NULL && text->strrep == OL_MB_STR_REP) {
		  if(!OlInsertOffsetsIntoPosTab(cpos, scpos, offset))
			return(EDIT_FAILURE);
		} else
			cpos->used += (scpos->used - 1);

		page->bytes += (sbuffer->used - null_len);
		page->chars += (scpos->used - 1);

		location->offset += (scpos->used - 1);

		return(EDIT_SUCCESS);
} /* end of OlInsertCharsIntoTextBufferAtLoc */

static EditResult 
OlInsertLineIntoTextBufferAtLoc(OlTextBuffer *text, 
					TextLocation *location,
		 			Buffer *sbuffer, 
		 			PositionTable *scpos)
{
TextLine at; 
TextPosition offset;
TextPosition byte_offset;
Buffer *buffer, *wbuf;
PositionTable *cpos, *wcpos;
OlPage *page;
int null_len = 0;
int basebyte = 0;
register int i = 0;

if(location == (TextLocation *)NULL)
   return (EDIT_FAILURE);
		
at = location->line;
page = &text->pages.p[text->lines.p[at].pageindex];
offset = location->offset;

if(offset == 0)
	return(OlInsertLineIntoTextBuffer(text,at,sbuffer,scpos));

   (void)OlGetTextBufferStringAtLine(text,at,(TextLocation *)NULL);
   buffer = text->buffer;
   cpos = text->cpos;

   wbuf = CopyBuffer(buffer);
   wcpos = (PositionTable *)CopyBuffer((Buffer *)cpos);

   switch(text->strrep) {
	case OL_MB_STR_REP:
		byte_offset = cpos->p[offset];
		buffer->p[byte_offset] = mbnull;
		null_len = mbnull_len;
		break;
	case OL_SB_STR_REP:
		byte_offset = offset;
		buffer->p[byte_offset] = '\0';
		null_len = 1;
		break;
	case OL_WC_STR_REP:
		byte_offset = offset*sizeof(wchar_t);
		memmove((XtPointer)&buffer->p[byte_offset],
			   (XtPointer)&wcnull, sizeof(wchar_t));
		null_len = sizeof(wchar_t);
		break;
    }
		/* adjust dn number of bytes in page */
		page->bytes -= (buffer->used - byte_offset - null_len); 
		buffer->used = byte_offset + null_len;

		/* adjust number of chars in page */
		page->chars -= (cpos->used - offset - 1);
		cpos->used = offset + 1;

		/* insert new line buffers */
		if(!OlInsertBytesIntoBuffer(buffer,sbuffer,
			sbuffer->used - null_len,byte_offset))
			return(EDIT_FAILURE);
		if(scpos->p != NULL && text->strrep == OL_MB_STR_REP)
		  if(!OlInsertOffsetsIntoPosTab(cpos, scpos, offset))
			return(EDIT_FAILURE);

		page->bytes += (sbuffer->used - null_len);
		page->chars += (scpos->used - 1);

		wbuf->used -= byte_offset;
		memmove((XtPointer)wbuf->p, 
			(XtPointer)&wbuf->p[byte_offset],wbuf->used);
		
		wcpos->used -= offset;
		if(wcpos->p != NULL && text->strrep == OL_MB_STR_REP) {
			basebyte = wcpos->p[offset];
			for(i= 0; i < wcpos->used; i++) 
				wcpos->p[i] = wcpos->p[i+offset] - basebyte;
		}

		location->line++;
		location->offset = 0;
		OlInsertLineIntoTextBuffer(text,location->line,wbuf,wcpos);

		return(EDIT_SUCCESS);
}


static int 
OlInsertBytesIntoBuffer(Buffer *target,
		      Buffer *source,
		      int    nbytes,
		      int    offset)
{
	if (offset < 0 || offset > target-> used || source-> used <= 0)
   		return(0);
	else {
   	int space_needed = nbytes - (target-> size - target-> used);

   	if (space_needed > 0)
      		GrowBuffer(target, space_needed + lnincre);
 
   	if (offset != target-> used)
      		memmove((XtPointer)&target->p[(offset + nbytes)],
              		(XtPointer)&target-> p[offset],
              		(target-> used - offset));
 
   	memmove((XtPointer)&target->p[offset],(XtPointer)source->p,nbytes);
   	target-> used += nbytes;

	}

return(1);

} /* end of OlInsertBytesIntoBuffer */

static int 
OlInsertOffsetsIntoPosTab(PositionTable *target,
			  PositionTable *source,
			  int 		offset)
{
int base = 0;
int nelems = source->used - 1;
int space_needed = 0;
register int i = 0;
PositionTable *work= (PositionTable *)CopyBuffer((Buffer *)target);

	if(source->p == NULL)
		return(1);

	if (offset < 0 || offset > target-> used || source-> used <= 0)
   		return(0);
	else {
		int space_needed = nelems - (target->size - target->used);

		if(space_needed > 0)
		     GrowBuffer((Buffer *)target, space_needed + linsiz);

		if(offset != target->used) {
			base = source->p[nelems];
			for(i = offset; i < target->used ; i++) 
				target->p[i+nelems] = work->p[i] + base; 		

			base = work->p[offset];
			for(i = 0; i < nelems; i++) {
				target->used++;
				target->p[i+offset] = source->p[i] + base;
			} /* end of for */

		} else {
			base = work->p[offset];
			for(i = 0; i < nelems; i++) {
				target->used++;
				target->p[i+offset] = source->p[i] + base;
			} /* end of for */
			
		} /* end of else */
	} 		

	FreeBuffer((Buffer *)work);

return(1);
} /* end of OlInsertIntoPistionTable */

static OlStr
MB_fwd_scan_func(OlStr string,
		 OlStr curp,
		 OlStr exp)
{
	return((OlStr)mbstrexp((char *)string,(char *)curp,
                                        (char *)exp));
}

static OlStr
SB_fwd_scan_func(OlStr string,
		 OlStr curp,
		 OlStr exp)
{
	return((OlStr)strexp((char *)string,(char *)curp,
					(char *)exp));
}

static OlStr
WC_fwd_scan_func(OlStr string,
		 OlStr curp,
		 OlStr exp)
{
        return((OlStr)wcstrexp((wchar_t *)string,(wchar_t *)curp,
                                                (wchar_t *)exp));
}

static OlStr
MB_bwd_scan_func(OlStr string,
		 OlStr curp,
		 OlStr exp)
{
       return((OlStr)mbstrrexp((char *)string,(char *)curp,
                                       (char *)exp));
}

static OlStr
SB_bwd_scan_func(OlStr string,
		 OlStr curp,
		 OlStr exp)
{
	return((OlStr)strrexp((char *)string,(char *)curp,
					(char *)exp));
}

static OlStr
WC_bwd_scan_func(OlStr string,
		 OlStr curp,
		 OlStr exp)
{
       return((OlStr)wcstrrexp((wchar_t *)string,(wchar_t *)curp,
                                       (wchar_t *)exp));
}


static TextPosition 
OlGetCharOffset(OlStr c,
		OlStr base,
		OlStrRep strrep) 
{
TextPosition offset = 0;

	switch(strrep) {
		case OL_SB_STR_REP:
			return((TextPosition)((char *)c - (char *)base));
		case OL_WC_STR_REP:
			return((TextPosition)
					(((char *)c - (char *)base)/sizeof(wchar_t)));
		case OL_MB_STR_REP:
			{
			unsigned char *p;
			int mbl = 0;

			for(p = (unsigned char *)base; p < (unsigned char *)c;
							p += mbl) { 
				mbl = mblen((char *)p, (size_t)((char *)c - 
								(char *)p));
				if(mbl != -1)
					offset++;
				else {
				   OlWarning(dgettext(OlMsgsDomain,
					"Invalid Multi Byte character \n"));
				   return(EOF);
				}

			 }
			 return(offset);
			 }
	} /* end of switch */

}


extern OlTextUndoItem
OlGetTextUndoInsertItem(OlTextBufferPtr text)
{
OlStrRep text_format;
OlTextUndoItem text_undo_item;
wchar_t *ws;

LockBuffer(text);
text_format = text->strrep;
text_undo_item = text->insert;
if(text_undo_item.string != (OlStr)NULL)
	switch(text_format) {
		case OL_SB_STR_REP:
		case OL_MB_STR_REP:
			text_undo_item.string = (OlStr)
				strdup((char *)
						text_undo_item.string);
			break;
		case OL_WC_STR_REP:
			ws = (wchar_t *)malloc((wslen((wchar_t *)
					text_undo_item.string)+1)*
					sizeof(wchar_t));
			wscpy(ws,(wchar_t *)text_undo_item.string);
			text_undo_item.string = (OlStr)ws;
			break;
	} /* switch */
UnlockBuffer(text);
return(text_undo_item);
}
			
extern void       
OlSetTextUndoInsertItem( 
			OlTextBufferPtr text, 
			OlTextUndoItem text_undo_insert)
{
OlStrRep text_format;
wchar_t *ws;

LockBuffer(text);
text_format = text->strrep;
text->insert = text_undo_insert;

if(text->insert.string != (OlStr)NULL)
	switch(text_format) {
		case OL_SB_STR_REP:
		case OL_MB_STR_REP:
			text->insert.string = (OlStr)
				strdup((char *)
						text->insert.string);
			break;
		case OL_WC_STR_REP:
			ws = (wchar_t *)malloc((wslen((wchar_t *)
					text->insert.string)+1)*
					sizeof(wchar_t));
			wscpy(ws,(wchar_t *)text->insert.string);
			text->insert.string = (OlStr)ws;
			break;
	} /* switch */
UnlockBuffer(text);
}
		
			
extern OlTextUndoItem
OlGetTextUndoDeleteItem(OlTextBufferPtr text)
{
OlStrRep text_format;
OlTextUndoItem text_undo_item;
wchar_t *ws;

LockBuffer(text);
text_format = text->strrep;
text_undo_item = text->deleted;
if(text_undo_item.string != (OlStr)NULL)
	switch(text_format) {
		case OL_SB_STR_REP:
		case OL_MB_STR_REP:
			text_undo_item.string = (OlStr)
				strdup((char *)
						text_undo_item.string);
			break;
		case OL_WC_STR_REP:
			ws = (wchar_t *)malloc((wslen((wchar_t *)
					text_undo_item.string)+1)*
					sizeof(wchar_t));
			wscpy(ws,(wchar_t *)text_undo_item.string);
			text_undo_item.string = (OlStr)ws;
			break;
	} /* switch */
UnlockBuffer(text);
return(text_undo_item);
}
			
extern void       
OlSetTextUndoDeleteItem( 
			OlTextBufferPtr text, 
			OlTextUndoItem text_undo_deleted)
{
OlStrRep text_format;
wchar_t *ws;

LockBuffer(text);
text_format = text->strrep;
text->deleted = text_undo_deleted;

if(text->deleted.string != (OlStr)NULL)
	switch(text_format) {
		case OL_SB_STR_REP:
		case OL_MB_STR_REP:
			text->deleted.string = (OlStr)
				strdup((char *)
						text->deleted.string);
			break;
		case OL_WC_STR_REP:
			ws = (wchar_t *)malloc((wslen((wchar_t *)
					text->deleted.string)+1)*
					sizeof(wchar_t));
			wscpy(ws,(wchar_t *)text->deleted.string);
			text->deleted.string = (OlStr)ws;
			break;
	} /* switch */
UnlockBuffer(text);
}

extern UnitPosition 
OlUnitPositionOfTextPosition(
                             OlTextBufferPtr text,
                             TextPosition pos)
{
TextLocation loc;
int byte_pos;
int lineindex;
UnitPosition position = EOF;
int j,i;

	LockBuffer(text);
	switch(text->strrep) {
		case OL_SB_STR_REP:
		case OL_WC_STR_REP:
			UnlockBuffer(text);
			return((UnitPosition)pos);
		case OL_MB_STR_REP:
			(void)OlLocationOfPosition(text,pos,&loc);
			if(OlGetTextBufferStringAtLine(text,
				loc.line,(TextLocation *)NULL) == NULL){
				UnlockBuffer(text);
				return EOF;
			}
			if(loc.offset >= text->cpos->used) {
				UnlockBuffer(text);
				return EOF;
			}
			byte_pos = text->cpos->p[loc.offset];
			lineindex = loc.line;
		if (lineindex >= (TextLine)0 && 
			lineindex < text-> lines.used) {
   			position = (UnitPosition)0;
   		for (i = (TextPage)0; i < text-> 
			lines.p[lineindex].pageindex; i++)
      			position += text-> pages.p[i].bytes;
   		for (j = OlFirstLineOfPage(text, i); j < lineindex; j++)
      			position += text->lines.p[j].buffer-> used;
   		}
		position += byte_pos;
		break;
	}

	UnlockBuffer(text);
	return(position);
}

extern UnitPosition 
OlUnitOffsetOfLocation(OlTextBufferPtr text,
			TextLocation *loc)
{
UnitPosition byte_position = EOF;

	LockBuffer(text);
	switch(text->strrep) {
		case OL_SB_STR_REP:
		case OL_WC_STR_REP:
			UnlockBuffer(text);
			return((UnitPosition)loc->offset);
		case OL_MB_STR_REP:
			if(OlGetTextBufferStringAtLine(text,
				loc->line,(TextLocation *)NULL) == NULL){
				UnlockBuffer(text);
				return EOF;
			}
			if(loc->offset >= text->cpos->used){
				UnlockBuffer(text);
				return EOF;
			}
			byte_position = 
				(UnitPosition)text->cpos->p[loc->offset];
			break;
	}

	UnlockBuffer(text);
	return(byte_position);
}

extern TextPosition 
OlCharOffsetOfUnitLocation(OlTextBufferPtr text,
				TextLocation *loc) /* unit location */
{
TextPosition char_offset = EOF;
int start, end , middle;

	LockBuffer(text);
	switch(text->strrep) {
		case OL_SB_STR_REP:
		case OL_WC_STR_REP:
			UnlockBuffer(text);
			return((TextPosition)loc->offset);
		case OL_MB_STR_REP:
			if(OlGetTextBufferStringAtLine(text,
				loc->line,(TextLocation *)NULL) == NULL){
				UnlockBuffer(text);
				return EOF;
			}
			if(loc->offset >= text->buffer->used) {
				UnlockBuffer(text);
				return EOF;
			}

			start = 0;
			end = text->cpos->used - 1;
			middle = (start + end) >> 1;
			while(text->cpos->p[middle] != loc->offset && end >= start) {

				if(text->cpos->p[middle] > loc->offset) 
					end = middle - 1;
				else
					start = middle + 1;

				middle = (start + end) >> 1;

			}
			if(end >= start)
				char_offset = middle;
			else
				char_offset  = EOF;
			break;
	} /* switch */

	UnlockBuffer(text);
	return(char_offset);
}

extern String 
OlGetTextBufferFileName(OlTextBufferPtr text)
{
String retval;

	LockBuffer(text);
	retval = (text->filename);
	UnlockBuffer(text);
	return retval;
}

extern Boolean
OlIsTextBufferModified(OlTextBufferPtr text)
{
Boolean retval;

	LockBuffer(text);
	retval = (text->dirty);
	UnlockBuffer(text);
	return retval;
}

extern Boolean
OlIsTextBufferEmpty(OlTextBufferPtr text)
{
Boolean retval;

	LockBuffer(text);
	retval = (text-> lines.used == 1 &&
                  text-> lines.p[0].cpos->used == 1);
	UnlockBuffer(text);
	return retval;
}

extern TextLine
OlLastTextBufferLine(OlTextBufferPtr text)
{
TextLine retval;

	LockBuffer(text);
	retval = ((TextLine)(text->lines.used -1));
	UnlockBuffer(text);
	return retval;
}

extern int
OlLastCharInTextBufferLine(OlTextBufferPtr text,
				TextLine line)
{
int retval;

	LockBuffer(text);
	retval = (text-> lines.p[line].cpos->used - 1);
	UnlockBuffer(text);
	return retval;
}

extern int
OlNumCharsInTextBufferLine(OlTextBufferPtr text,
				TextLine line)
{
int retval;

	LockBuffer(text);
	retval = (text->lines.p[line].cpos->used);
	UnlockBuffer(text);
	return retval;
}

extern int
OlNumBytesInTextBufferLine(OlTextBufferPtr text,
				TextLine line)
{
int retval;

	LockBuffer(text);
	retval = (text->lines.p[line].buffer->used);
	UnlockBuffer(text);
	return retval;
}

extern int
OlNumUnitsInTextBufferLine(OlTextBufferPtr text,
				TextLine line)
{
int retval;

	LockBuffer(text);
	retval = ((text->strrep == OL_MB_STR_REP ?
			text->lines.p[line].buffer->used:
			text->lines.p[line].cpos->used));
	UnlockBuffer(text);
	return retval;
}
