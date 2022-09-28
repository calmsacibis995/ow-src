#pragma	ident	"@(#)textbuff.c	302.9	97/03/26 lib/libXol SMI"	/* olmisc:textbuff.c 1.7	*/

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


#include <stdio.h>
#include <string.h>
#ifdef MTSAFE
#include <synch.h>
#include <thread.h>
#include <assert.h>
#endif /* MTSAFE */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/buffutil.h>
#include <Xol/memutil.h>
#include <Xol/regexp.h>
#include <Xol/textbuff.h>


#ifdef MTSAFE
#define NO_THREAD 0
#define LockBuffer()	LockBuffer()
#define UnlockBuffer()	UnlockBuffer()

typedef struct _buffer_lock {
	mutex_t			lock;		
	thread_t		thread_id;
	cond_t			cond_var;
	int			recursion;
}	BufferLock;
static BufferLock lock_buf = {0, 0, 0, 0};

static void 
LockBuffer()
{
	mutex_lock(&lock_buf.lock);
	if(lock_buf.thread_id == thr_self()) {
		lock_buf.recursion++;
		mutex_unlock(&lock_buf.lock);
		return;
	}

	if(lock_buf.thread_id == NO_THREAD) {
		assert(lock_buf.recursion == 0);
		lock_buf. thread_id = thr_self();
		mutex_unlock(&lock_buf.lock);
		return;
	} 

	while(lock_buf.thread_id != NO_THREAD)
		cond_wait(&lock_buf.cond_var, &lock_buf.lock);	

	assert(lock_buf.thread_id == NO_THREAD);
	lock_buf.thread_id = thr_self();
	mutex_unlock(&lock_buf.lock);
	
}

static void
UnlockBuffer()
{
	mutex_lock(&lock_buf.lock);
	assert(lock_buf.thread_id == thr_self());
	if(lock_buf.recursion == 0) 
		lock_buf.thread_id = NO_THREAD;
	else
		lock_buf.recursion--;
	mutex_unlock(&lock_buf.lock);
}
#else
#define LockBuffer()
#define UnlockBuffer()
#endif /*MTSAFE*/

/* TextBuffer table sizes and increments */
#define FTALLOC         8
#define LTALLOC         2
#define PTALLOC         4
#define BTALLOC         8
#define MAXLPP         96
#define BLOCKSIZE    2048


#define IS_A_WORD_CHAR(c)	(c != '\0' && (*IsAWordChar)(c))

#define IS_NOT_A_WORD_CHAR(c)	(c == '\0' || !(*IsAWordChar)(c))

#define AppendLineToTextBuffer(text, b) \
   InsertLineIntoTextBuffer(text, (text-> lines.used), b)

static TextPage		NewPage(TextBuffer *text, TextPage after);
static TextPage		SplitPage(TextBuffer *text, TextPage after, TextLine line);
static void		SwapIn(TextBuffer *text, TextPage pageindex);
static void		SwapOut(TextBuffer *text, TextPage pageindex);
static TextLine		FirstLineOfPage(TextBuffer *text, TextPage page);
static void		AllocateBlock(TextBuffer *text, TextPage pageindex);
static void		FreeBlock(TextBuffer *text, TextPage pageindex, TextBlock blockindex);
static void		InitTextUndoList(TextBuffer *text);
static void		FreeTextUndoList(TextBuffer *text);

static EditResult	InsertBlockIntoTextBuffer(TextBuffer *text, TextLocation *location, char *string);
static EditResult	InsertLineIntoTextBuffer(TextBuffer *text, TextLine at, Buffer *buffer);
static EditResult	DeleteBlockInTextBuffer(TextBuffer *text, TextLocation *startloc, TextLocation *endloc);
static EditResult	DeleteLineInTextBuffer(TextBuffer *text, TextLine at);
static int		is_a_word_char(int rc);


static TextPosition	textblockavailable = 0;

static OlStrScanDefProc	Strexp = (OlStrScanDefProc)strexp;
static OlStrScanDefProc	Strrexp = (OlStrScanDefProc)strrexp;
static OlStrWordDefProc	IsAWordChar = (OlStrWordDefProc)is_a_word_char;


static void
printbuffer (TextBuffer *text)
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

(void) fprintf(stderr, " line  page   len   slen size\n");
for (i = 0; i < text-> lines.used; i++)
   {
   (void) fprintf(stderr,"%5d %5d %5d %5d %5d\n", 
                  i, 
                  text-> lines.p[i].pageindex,
                  text-> lines.p[i].buffer-> used,
                  strlen(text-> lines.p[i].buffer-> p),
                  text-> lines.p[i].buffer-> size);
   }

} /* end of printbuffer */

/*
 * AllocateTextBuffer
 *
 * The \fIAllocateTextBuffer\fR function is used to allocate a new TextBuffer.
 * After it allocates the structure itself, initializes
 * the members of the structure, allocating storage, setting
 * initial values, etc.
 * The routine also registers the update function provided by the caller.
 * This function normally need not be called by an application developer 
 * since the \fIReadFileIntoTextBuffer\fR and
 * \fIReadStringIntoTextBuffer\fR functions call this routine
 * before starting their operation.
 * The routine returns a pointer to the allocated TextBuffer.
 *
 * The \fIFreeTextBuffer\fR function should be used to deallocate
 * the storage allocated by this routine.
 *
 * See also:
 *
 * FreeTextBuffer(3), ReadFileIntoTextBuffer(3), ReadStringIntoTextBuffer(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextBuffer *
AllocateTextBuffer (char *filename, TextUpdateFunction f, caddr_t d)
{
TextBuffer * text = (TextBuffer *) MALLOC(sizeof(TextBuffer));

text-> filename = filename == NULL ?
       NULL : strcpy(MALLOC((unsigned)(strlen(filename) + 1)), filename);
text-> tempfile      = NULL;
text-> blockcnt      = (TextBlock)0;
text-> lines.used    = (TextLine)0;
text-> lines.size    = LTALLOC;
text-> lines.esize   = sizeof(Line);
text-> lines.p       = (Line *) MALLOC((unsigned)(sizeof(Line) * LTALLOC));
text-> pages.used    = (TextPage)0;
text-> pages.size    = PTALLOC;
text-> pages.esize   = sizeof(Page);
text-> pages.p       = (Page *) MALLOC((unsigned)(sizeof(Page) * PTALLOC));
text-> free_list     = (BlockTable *)AllocateBuffer(sizeof(TextBlock), FTALLOC);
text-> pagecount     = (TextPage)0;
text-> pageref       = (TextPage)0;
text-> curpageno     = NewPage(text, -1);
text-> buffer        = NULL; 
                       /* AllocateBuffer(sizeof(text->buffer->p[0]), LNMIN); */
text-> dirty         = 0;
text-> status        = NOTOPEN;
text-> update        = (TextUpdateCallback *)NULL;
text-> refcount      = 0;
RegisterTextBufferUpdate(text, f, d);
InitTextUndoList(text);

return (text);

} /* end of AllocateTextBuffer */
/*
 * FreeTextBuffer
 *
 * The \fIFreeTextBuffer\fR procedure is used to deallocate storage
 * associated with a given TextBuffer.  Note: the storage is not
 * actually freed if the TextBuffer is still associated with other
 * update function/data pairs.
 *
 * See also:
 *
 * AllocateTextBuffer(3), RegisterTextBufferUpdate(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern void
FreeTextBuffer (TextBuffer *text, TextUpdateFunction f, caddr_t d)
{
register TextPage i;
register TextLine j;
register TextPage pageindex;

LockBuffer();
(void) UnregisterTextBufferUpdate(text, f, d);

if (text-> refcount == 0)
   {
   for (i = 0; i < text-> lines.used; i++)
      {
      if (text-> lines.p[i].buffer-> p != NULL)
         FREE(text-> lines.p[i].buffer-> p);
      FREE(((char *)text-> lines.p[i].buffer));
      }

   for (i = 0; i < text-> pages.used; i++)
      if (text-> pages.p[i].dpos != NULL)
         FreeBuffer((Buffer *)text-> pages.p[i].dpos);

   FREE(((char *)text-> lines.p));
   FREE(((char *)text-> pages.p));
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

   FREE(((char *)text));
   }

UnlockBuffer();
} /* FreeTextBuffer */

/*
 * ReadStringIntoTextBuffer
 *
 * The \fIReadStringIntoTextBuffer\fR function is used to copy the
 * given \fIstring\fR into a newly allocated TextBuffer.  The
 * supplied TextUpdateFunction and data pointer are associated
 * with this TextBuffer.
 *
 * See also:
 *
 * ReadFileIntoTextBuffer(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextBuffer *
ReadStringIntoTextBuffer (char *string, TextUpdateFunction f, caddr_t d)
{
Buffer * sp;
Buffer * work;
TextBuffer * text = AllocateTextBuffer(NULL, f, d);

LockBuffer();
sp = stropen(string);
if (sp != NULL)
   {
   work = AllocateBuffer(sizeof(work-> p[0]), LNMIN); 

   while(ReadStringIntoBuffer(sp, work) != EOF)
      (void) AppendLineToTextBuffer(text, work);
   (void) AppendLineToTextBuffer(text, work);

   FreeBuffer(work);
   strclose(sp);
   (void) GetTextBufferLocation(text, 0, (TextLocation *)NULL);
   }

UnlockBuffer();
return (text);

} /* end of ReadStringIntoTextBuffer */
/*
 * ReadFileIntoTextBuffer
 *
 * The \fIReadFileIntoTextBuffer\fR function is used to read the
 * given \fIfile\fR into a newly allocated TextBuffer.  The
 * supplied TextUpdateFunction and data pointer are associated
 * with this TextBuffer.
 *
 * See also:
 *
 * ReadStringIntoTextBuffer(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextBuffer *
ReadFileIntoTextBuffer (char *filename, TextUpdateFunction f, caddr_t d)
{
FILE * fp;
TextBuffer * text = NULL;
TextFileStatus status = READWRITE;
Buffer * work;

LockBuffer();
status = READWRITE;
if ((fp = fopen(filename, "rw")) == NULL)
   {
   status = READONLY;
   if ((fp = fopen(filename, "r")) == NULL)
      status = NEWFILE;
   }

if (fp != NULL)
   {
   text = AllocateTextBuffer(filename, f, d);
   text-> status = status;

   work = AllocateBuffer(sizeof(work-> p[0]), LNMIN); 

#ifndef DEBUG_TEXTBUFF
   while(ReadFileIntoBuffer(fp, work) != EOF)
      (void) AppendLineToTextBuffer(text, work);
#else
   {
   int i = 0;
   while(ReadFileIntoBuffer(fp, work) != EOF) {
      ++i;
      if (i%100 == 0) {
         fprintf(stderr,
            "at i = %d, pagecount = %d, curpageno = %d, pagesused = %d\n",
            i, text-> pagecount, text-> curpageno, text-> pages.used);
      }
      (void) AppendLineToTextBuffer(text, work);
   }
   }
#endif
   if (text-> lines.used == 0 && work-> used == 0)
      {
      work-> used = 1;
      work-> p[0] = '\0';
      (void) AppendLineToTextBuffer(text, work);
      }
   else
      if (work-> used != 0)
         (void) AppendLineToTextBuffer(text, work);

   FreeBuffer(work);
   (void) fclose(fp);
   (void) GetTextBufferLocation(text, 0, (TextLocation *)NULL);
   }

UnlockBuffer();
return (text);

} /* end of ReadFileIntoTextBuffer */
/*
 * GetTextBufferLocation
 *
 * The \fIGetTextBufferLocation\fR function is used to retrieve
 * the contents of the given line within the TextBuffer.  It
 * returns a pointer to the character string.  If the line
 * number is invalid a NULL pointer is returned.  If a non-NULL TextLocation
 * pointer is supplied in the argument list the contents of this
 * structure are modified to reflect the values corresponding to
 * the given line.
 *
 * See also:
 *
 * GetTextBufferBlock(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern char *
GetTextBufferLocation (TextBuffer *text, TextLine line_number, TextLocation *location)
{
char *retval;

LockBuffer();
if (line_number < 0 || line_number >= text-> lines.used) {
   UnlockBuffer();
   return (NULL);
}

if (text-> lines.p[line_number].pageindex != text-> curpageno)
   SwapIn(text, text-> lines.p[line_number].pageindex);

text-> buffer = text-> lines.p[line_number].buffer;

if (location)
   {
   location-> line   = line_number;
   location-> offset = 0;
   location-> buffer = text-> buffer-> p;
   }

retval = text-> buffer-> p;
UnlockBuffer();
return retval;

} /* GetTextBufferLocation */

/*
 * ForwardScanTextBuffer
 *
 * The \fIForwardScanTextBuffer\fR function is used to scan,
 * towards the end of the buffer,
 * for a given \fIexp\fRression in the TextBuffer starting
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
 * BackwardScanTextBuffer(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern ScanResult
ForwardScanTextBuffer (TextBuffer *text, TextLocation *location, char *exp)
{
char * c;
TextLine s = location-> line;
ScanResult retval = SCAN_INVALID;

LockBuffer();
if (exp != NULL)
   {
   retval = SCAN_FOUND;
   (void) GetTextBufferLocation(text, s, (TextLocation *)NULL);
   if ((c = Strexp(text-> buffer-> p,
                  &text-> buffer-> p[location-> offset], exp)) == NULL)
      {
      for (s = location-> line + 1; s < text-> lines.used; s++)
         {
         (void) GetTextBufferLocation(text, s, (TextLocation *)NULL);
         if ((c = Strexp(text-> buffer-> p, text-> buffer-> p, exp)) != NULL)
            break;
         }

      if (s == text-> lines.used)
         {
         retval = SCAN_WRAPPED;
         for (s = 0; s <= location-> line; s++)
            {
            (void) GetTextBufferLocation(text, s, (TextLocation *)NULL);
            if ((c = Strexp(text-> buffer-> p,
                            text-> buffer-> p, exp)) != NULL)
               break;
            }
         if (s > location-> line || (s == location-> line &&
            (c == NULL || c - text-> buffer-> p >= location-> offset)))
            retval = SCAN_NOTFOUND;
         }
      }
   if (retval == SCAN_FOUND || retval == SCAN_WRAPPED)
      {
      location-> line   = s;
      location-> offset = c - text-> buffer-> p;
      location-> buffer = text-> buffer-> p;
      }
   }
UnlockBuffer();
return (retval);

} /* end of ForwardScanTextBuffer */
/*
 * BackwardScanTextBuffer
 *
 * The \fIBackwardScanTextBuffer\fR function is used to scan,
 * towards the beginning of the buffer,
 * for a given \fIexp\fRression in the TextBuffer starting
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
 * ForwardScanTextBuffer(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern ScanResult
BackwardScanTextBuffer (TextBuffer *text, char *exp, TextLocation *location)
{
char * c;
TextLine s = location-> line;
ScanResult retval = SCAN_INVALID;

LockBuffer();
if (exp != NULL)
   {
   retval = SCAN_FOUND;
   (void) GetTextBufferLocation(text, s, (TextLocation *)NULL);
   if ((c = Strrexp(text-> buffer-> p,
        &text-> buffer-> p[location-> offset - 1], exp)) == NULL)
      {
      for (s = location-> line - 1; s >= (TextLine)0; s--)
         {
         (void) GetTextBufferLocation(text, s, (TextLocation *)NULL);
         if ((c = Strrexp(text-> buffer-> p, NULL, exp)) != NULL)
            break;
         }

      if (s < 0)
         {
         retval = SCAN_WRAPPED;
         for (s = text-> lines.used - 1; s > location-> line; s--)
            {
            (void) GetTextBufferLocation(text, s, (TextLocation *)NULL);
            if ((c = Strrexp(text-> buffer-> p, NULL, exp)) != NULL)
               break;
            }

         if (s == location-> line &&
            (c == NULL || c - text-> buffer-> p <= location-> offset))
            retval = SCAN_NOTFOUND;
         }
      }
   if (retval == SCAN_FOUND || retval == SCAN_WRAPPED)
      {
      location-> line   = s;
      location-> offset = c - text-> buffer-> p;
      location-> buffer = text-> buffer-> p;
      }
   }

UnlockBuffer();
return (retval);

} /* end of BackwardScanTextBuffer */
/*
 * NewPage
 *
 */

static TextPage
NewPage (TextBuffer *text, TextPage after)
{
TextPage at = after + 1;
register TextPage i = text-> pagecount;
register TextPage j;

#ifdef DEBUG_TEXTBUFF
   (void) fprintf(stderr,"allocating a new page at %d\n", at);
#endif

if (i == PQLIMIT)
   {
   for (i = 0, j = 1; j < PQLIMIT; j++)
      if (text-> pqueue[j].timestamp < text-> pqueue[i].timestamp)
         i = j;

   SwapOut(text, text-> pqueue[i].pageindex);
   }
else
   text-> pagecount++;

if (text-> pages.used == text-> pages.size)
   {
   text-> pages.size *= 2;
   text-> pages.p = (Page *) REALLOC
      ((char *)text-> pages.p, (unsigned)(text-> pages.size * sizeof(Page)));
   }

if (at < text-> pages.used++)
   {
   for (j = FirstLineOfPage(text, at); j < text-> lines.used; j++)
      text-> lines.p[j].pageindex++;

   for (j = 0; j < text-> pagecount; j++)
      if (text-> pqueue[j].pageindex >= at)
         text-> pqueue[j].pageindex++;
   memmove(&text-> pages.p[at + 1], &text-> pages.p[at],
           (text-> pages.used - at  - 1) * sizeof(Page));
   }

text-> pages.p[at].bytes = (TextPosition)0;
text-> pages.p[at].lines = (TextLine)0;
text-> pages.p[at].qpos  = i;
text-> pages.p[at].dpos  = (BlockTable *)NULL;

text-> pqueue[i].pageindex = text-> curpageno = at;
text-> pqueue[i].timestamp = text-> pageref++;

#ifdef DEBUG_TEXTBUFF
printbuffer(text);
(void) fprintf(stderr,"after adding a page at %d\n\n", at);
#endif

return (at);

} /* end of NewPage */
/*
 * SplitPage
 *
 */

static TextPage
SplitPage (TextBuffer *text, TextPage after, TextLine line)
{
TextPage newpage;
TextLine l;
TextLine s;

/*
if (line == text-> lines.used)
   {
   (void) fprintf(stderr,"adding page after %d\n", after);
   newpage = NewPage(text, after);
   }
else
*/
   {
#ifdef DEBUG_TEXTBUFF
   printbuffer(text);
   (void) fprintf(stderr,"splitting page %d\n", after);
#endif
   if (text-> curpageno != after)
      SwapIn(text, after);
   s = l = FirstLineOfPage(text, after) + MAXLPP / 2;
   newpage = NewPage(text, after);
   while (l < text-> lines.used && text-> lines.p[l].pageindex == after)
      {
      text-> pages.p[after].bytes -= text-> lines.p[l].buffer-> used;
      text-> pages.p[after].lines--;
      l++;
      }
#ifdef DEBUG_TEXTBUFF
   (void) fprintf(stderr,"AFTER = %d NEWPAGE = %d   ...", after, newpage);
#endif
   l = s;
   while (l < text-> lines.used && text-> lines.p[l].pageindex == after)
      {
      text-> lines.p[l].pageindex = newpage;
      text-> pages.p[newpage].bytes += text-> lines.p[l].buffer-> used;
      text-> pages.p[newpage].lines++;
      l++;
      }
   newpage = (text-> lines.used ==  (TextLine)0 ? (TextPage)0 :
              text-> lines.used == line ? text-> lines.p[line - 1].pageindex :
                                          text-> lines.p[line].pageindex);

#ifdef DEBUG_TEXTBUFF
   (void) fprintf(stderr,"l: %d newpage = %d  curpage = %d\n", 
                  line, newpage, text-> curpageno);
   printbuffer(text);
#endif
   if (text-> curpageno != newpage)
      SwapIn(text, newpage);
   }

return (newpage);

} /* end of SplitPage */
/*
 * SwapIn
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
SwapIn (TextBuffer *text, TextPage pageindex)
{
register TextPage  i = text-> pages.p[pageindex].qpos;
register TextPage  j;
register TextBlock block = 0;
register TextLine  n;
register BufferElement * p;

#ifdef DEBUG_TEXTBUFF
(void) fprintf(stderr, "swapping in %d\n", pageindex);
#endif

if (i == PQLIMIT)
   {
   if (text-> pagecount == PQLIMIT)
      {
      for (i = 0, j = 1; j < PQLIMIT; j++)
         if (text-> pqueue[j].timestamp < text-> pqueue[i].timestamp)
            i = j;
      SwapOut(text, text-> pqueue[i].pageindex);
      }
   else
      {
      i = text-> pagecount;
      text-> pagecount++;
      }

   text-> pages.p[pageindex].qpos = i;

   FreeBlock(text, pageindex, block++);
   for (j = FirstLineOfPage(text, pageindex);
        text-> lines.p[j].pageindex == pageindex && j < text-> lines.used; j++)
      {
      text-> lines.p[j].buffer-> size =
      n = text-> lines.p[j].buffer-> used;
      p = text-> lines.p[j].buffer-> p =
         (BufferElement *) MALLOC((unsigned)(n * sizeof(BufferElement)));
      while (n > textblockavailable)
         {
         (void) fread(p, 1, textblockavailable, text-> tempfile);
         n -= textblockavailable;
         p = &p[textblockavailable];
         FreeBlock(text, pageindex, block++);
         }
      if (n > 0)
         {
         (void) fread(p, 1, n, text-> tempfile);
         textblockavailable -= n;
         }
      }
   text-> pages.p[pageindex].dpos-> used = 0;
   }

text-> pqueue[i].pageindex = text-> curpageno = pageindex;
text-> pqueue[i].timestamp = text-> pageref++;

} /* end of SwapIn */
/*
 * SwapOut
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
SwapOut (TextBuffer *text, TextPage pageindex)
{
register TextLine i;
register TextPosition n;
register BufferElement * p;

#ifdef DEBUG_TEXTBUFF
(void) fprintf(stderr, "swapping out %d\n", pageindex);
#endif

if (pageindex == -1)
   return;

if (text-> tempfile == NULL)
   {
   text-> blocksize = BLOCKSIZE;
   text-> tempfile = tmpfile();
   }

if (text-> pages.p[pageindex].dpos == (BlockTable *)NULL)
   text-> pages.p[pageindex].dpos =
      (BlockTable *) AllocateBuffer(sizeof(TextBlock), BTALLOC);

AllocateBlock(text, pageindex);
for (i = FirstLineOfPage(text, pageindex);
     text-> lines.p[i].pageindex == pageindex && i < text-> lines.used; i++)
   {
   p = text-> lines.p[i].buffer-> p;
   n = text-> lines.p[i].buffer-> used;
   while (n > textblockavailable)
      {
      (void) fwrite(p, 1, textblockavailable, text-> tempfile);
      n -= textblockavailable;
      p = &p[textblockavailable];
      AllocateBlock(text, pageindex);
      }
   if (n > 0)
      {
      (void) fwrite(p, 1, n, text-> tempfile);
      textblockavailable -= n;
      }
   FREE(text-> lines.p[i].buffer-> p);
   text-> lines.p[i].buffer-> p = NULL;
   }

text-> pages.p[pageindex].qpos = PQLIMIT;

} /* end of SwapOut */
static TextLine
FirstLineOfPage (TextBuffer *text, TextPage page)
{
register TextPage i;
register TextLine l = 0;

for (i = 0; i < page; i++)
   l += text-> pages.p[i].lines;

return (l);

} /* end of FirstLineOfPage */


/*
 * AllocateBlock
 *
 */
static void
AllocateBlock (TextBuffer *text, TextPage pageindex)
{
TextBlock block;

if (BufferEmpty(text-> free_list))
   block = text-> blockcnt++;
else
   block = text-> free_list-> p[--text-> free_list-> used];

(void) fseek(text-> tempfile, (long)(block * text-> blocksize), SEEK_SET);
textblockavailable = text-> blocksize;

if (BufferFilled(text->pages.p[pageindex].dpos))
   GrowBuffer((Buffer *)text->pages.p[pageindex].dpos, 66);

text-> pages.p[pageindex].dpos-> p[text-> pages.p[pageindex].dpos-> used++] =
   block;

} /* end of AllocateBlock */


/*
 * FreeBlock
 *
 */

static void FreeBlock (TextBuffer *text, TextPage pageindex, TextBlock blockindex)
{
TextBlock block = text-> pages.p[pageindex].dpos-> p[blockindex];

if (BufferFilled(text-> free_list))
   GrowBuffer((Buffer *)text-> free_list, text-> free_list-> size * 2);
text-> free_list-> p[text-> free_list-> used++] = block;

(void) fseek(text-> tempfile, (long)(block * text-> blocksize), SEEK_SET);
textblockavailable = text-> blocksize;

} /* end of FreeBlock */
/*
 * InsertBlockIntoTextBuffer
 *
 */

static EditResult
InsertBlockIntoTextBuffer (TextBuffer *text, TextLocation *location, char *string)
{
Buffer * sp;
Buffer * buffer;
static Buffer * work;
TextLine lineindex = location-> line;
TextPage pageindex;

if (work == NULL)
   work = AllocateBuffer(sizeof(work->p[0]), LNMIN);

if (string == NULL || string[0] == '\0')
   return (EDIT_SUCCESS);

if (location-> line >= (TextLine)0 && location-> line < text-> lines.used &&
    location-> offset >= (TextLine)0 &&
    location-> offset < text-> lines.p[lineindex].buffer-> used)
   {
   sp = stropen(string);
   while (ReadStringIntoBuffer(sp, work) != EOF)
      {
      if (location->offset == 0)
         {
#ifdef VERBOSE
         (void) fprintf(stderr,"inserting a line at %d\n", location-> line);
#endif
         text-> insert.hint |= TEXT_BUFFER_INSERT_LINE;
         InsertLineIntoTextBuffer(text, location-> line, work);
         location-> line++;
         location-> offset = 0;
         }
      else
         {
         Buffer * work2;

#ifdef VERBOSE
         (void) fprintf(stderr,"splitting a line at %d\n", location-> line);
#endif
         text-> insert.hint |= TEXT_BUFFER_INSERT_SPLIT_LINE;

         (void) GetTextBufferLocation(text, location-> line,
         	(TextLocation *)NULL);
         buffer = text-> lines.p[location-> line].buffer;
         work2 = CopyBuffer(buffer);
         text-> pages.p[text-> lines.p[location-> line].pageindex].bytes -=
            (buffer-> used - location-> offset - 1);
         buffer-> used = location-> offset + 1;
         buffer-> p[location-> offset] = '\0';

         InsertIntoBuffer(buffer, work, location-> offset);
         text-> pages.p[text-> lines.p[location-> line].pageindex].bytes +=
            (work-> used - 1);

         location-> line++;

         work2-> used -= location-> offset;
         memmove(work2-> p, &work2-> p[location-> offset], work2-> used);
         InsertLineIntoTextBuffer(text, location-> line, work2);
         FreeBuffer(work2);

         location-> offset = 0;
         }
      }
   if (work-> used > 1)
      {
#ifdef VERBOSE
      (void) fprintf(stderr,"inserting '%s' at %d.%d\n",
                     work-> p, location-> line, location-> offset);
#endif
      text-> insert.hint |= TEXT_BUFFER_INSERT_CHARS;
      (void) GetTextBufferLocation(text, location-> line, (TextLocation *)NULL);
      buffer = text-> lines.p[location-> line].buffer;
      InsertIntoBuffer(buffer, work, location-> offset);
      location-> offset += (work-> used - 1);
      text-> pages.p[text-> lines.p[location-> line].pageindex].bytes +=
         (work-> used - 1);
      }
   strclose(sp);
   return (EDIT_SUCCESS);
   }
else
   return (EDIT_FAILURE);

} /* end of InsertBlockIntoTextBuffer */
/*
 * InsertLineIntoTextBuffer
 *
 * This function insert a given Buffer into a given TextBuffer at
 * a given line.  The algorithm used is:	
 *
 *  1. Check for line within range, if not return failure else continue.
 *  2. Determine if affected page has room for another line, if not
 *     allocate a new page.
 *  3. If page is not the current page then SwapIn the page.
 *  4. Increase the LineTable and shift down if necessary.
 *  5. Copy the Buffer into a newly allocated buffer.
 *  6. Store this buffer pointer and the pageindex in the line table.
 *  7. Increment the Page bytes and lines.
 *  8. Return success.
 */

static EditResult
InsertLineIntoTextBuffer (TextBuffer *text, TextLine at, Buffer *buffer)
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

   if (text-> pages.p[page].lines == MAXLPP)
      page = SplitPage(text, page, at);
   else
      if (text-> curpageno != page)
         SwapIn(text, page);

   if (text-> lines.used == text-> lines.size)
      {
      text-> lines.size *= 2;
      text-> lines.p = (Line *) REALLOC
         ((char *)text-> lines.p, (unsigned)(text-> lines.size * sizeof(Line)));
      }

   if (at < text-> lines.used++)
      memmove(&text-> lines.p[at + 1], &text-> lines.p[at],
              (text-> lines.used - at - 1) * sizeof(Line));

   savesize = buffer-> size; 
   buffer-> size = buffer-> used;
   text-> lines.p[at].buffer    = CopyBuffer(buffer);
   buffer-> size = savesize;
   text-> lines.p[at].pageindex = page;
   text-> lines.p[at].userData  = 0L;

   text-> pages.p[page].bytes += text-> lines.p[at].buffer-> used;
   text-> pages.p[page].lines++;

   return (EDIT_SUCCESS);
   }

} /* end of InsertLineIntoTextBuffer */
/**************************************************************/
/**************************************************************/
/**************************************************************/
/*
 * DeleteBlockInTextBuffer
 *
 */

static EditResult
DeleteBlockInTextBuffer (TextBuffer *text, TextLocation *startloc, TextLocation *endloc)
{
Buffer * work;
char * p;
register int len;
register int i;
register int j;
register int k;

if (startloc-> line == endloc-> line)
   {
#ifdef VERBOSE
   (void) fprintf(stderr," deleteing in line\n");
#endif
   len = endloc-> offset - startloc-> offset;
   if (len > 0)
      if (len == text-> lines.p[endloc-> line].buffer-> used)
         {
(void) fprintf(stderr,"delete a line!\n");
         }
      else
         {
#ifdef VERBOSE
         (void) fprintf(stderr,"simple delete of chars in line\n");
#endif
         text-> deleted.hint = TEXT_BUFFER_DELETE_SIMPLE;
         p = GetTextBufferLocation(text, startloc-> line, (TextLocation *)NULL);
         memmove(&p[startloc-> offset],
                 &p[endloc-> offset],
                 text-> lines.p[endloc-> line].buffer-> used - endloc-> offset);
         text-> lines.p[endloc-> line].buffer-> used -= len;
         text-> pages.p[text-> lines.p[endloc-> line].pageindex].bytes -= len;
         }
   }
else
   {
   if (startloc-> offset == 0)
      {
      i = startloc-> line;
      text-> deleted.hint |= TEXT_BUFFER_DELETE_START_LINE;
      }
   else
      {
#ifdef VERBOSE
      (void) fprintf(stderr," deleting at start\n");
#endif
      text-> deleted.hint |= TEXT_BUFFER_DELETE_START_CHARS;
      p = GetTextBufferLocation(text, startloc-> line, (TextLocation *)NULL);
      len = text-> lines.p[startloc-> line].buffer-> used - startloc-> offset;
      text-> lines.p[startloc-> line].buffer-> used -= (len - 1);
      text-> pages.p[text-> lines.p[startloc-> line].pageindex].bytes -= (len - 1);
      p[startloc-> offset] = '\0';
      i = startloc-> line + 1;
      }

   if (endloc-> offset == text-> lines.p[endloc-> line].buffer-> used)
      {
      k = endloc-> line;
      text-> deleted.hint |= TEXT_BUFFER_DELETE_END_LINE;
      }
   else
      {
      if (endloc-> offset != 0)
         {
#ifdef VERBOSE
         (void) fprintf(stderr," deleting on end\n");
#endif
         text-> deleted.hint |= TEXT_BUFFER_DELETE_END_CHARS;
         p = GetTextBufferLocation(text, endloc-> line, (TextLocation *)NULL);
         len = endloc-> offset;
         text-> lines.p[endloc-> line].buffer-> used -= len;
         text-> pages.p[text-> lines.p[endloc-> line].pageindex].bytes -= len;
         memmove(p, &p[len], text-> lines.p[endloc-> line].buffer-> used);
         }
      k = endloc-> line - 1;
      }

   for (j = i; j <= k; j++)
      DeleteLineInTextBuffer(text, i);

   if (i != startloc-> line && k != endloc-> line)
      {
#ifdef VERBOSE
      (void) fprintf(stderr," joining lines\n");
#endif
      text-> deleted.hint |= TEXT_BUFFER_DELETE_JOIN_LINE;
      j = startloc-> line;
      (void) GetTextBufferLocation(text, j, (TextLocation *)NULL);
      work = CopyBuffer(text-> buffer);
      DeleteLineInTextBuffer(text, j);

      (void) GetTextBufferLocation(text, j, (TextLocation *)NULL);
      InsertIntoBuffer(text-> buffer, work, 0);
      text-> pages.p[text-> lines.p[j].pageindex].bytes += (work-> used - 1);

      FreeBuffer(work);
      }
   }

#ifdef DEBUG_TEXTBUFF
(void) fprintf(stderr,"end of delete\n\n");
#endif

return (EDIT_SUCCESS);

} /* end of DeleteBlockInTextBuffer */
/*
 * DeleteLineInTextBuffer
 *
 */

static EditResult
DeleteLineInTextBuffer (TextBuffer *text, TextLine at)
{
TextPage pageindex;
TextLine j;

if (at < 0 || at >= text-> lines.used)
   return (EDIT_FAILURE);
else
   {
   pageindex = text-> lines.p[at].pageindex;

   if (text-> lines.p[at].pageindex != text-> curpageno)
      SwapIn(text, pageindex);

   text-> pages.p[pageindex].bytes -= text-> lines.p[at].buffer-> used;
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
#ifdef DEBUG_TEXTBUFF
         printbuffer(text);
         fprintf(stderr, "free page\n");
#endif

         memmove(&text-> pages.p[pageindex],
                 &text-> pages.p[pageindex + 1],
                 (text-> pages.used - pageindex) * sizeof(Page));
         for (j = FirstLineOfPage(text, pageindex); j < text-> lines.used; j++)
            text-> lines.p[j].pageindex--;
         for (j = 0; j < text-> pagecount; j++)
            if (text-> pqueue[j].pageindex > pageindex)
               text-> pqueue[j].pageindex--;
#ifdef DEBUG_TEXTBUFF
         printbuffer(text);
#endif
         }
      }

   FreeBuffer((Buffer *)text-> lines.p[at].buffer);

   if (at != --text-> lines.used)
      memmove(&text-> lines.p[at], &text-> lines.p[at + 1],
              (text-> lines.used - at) * sizeof(Line));

   return (EDIT_SUCCESS);
   }

} /* DeleteLineInTextBuffer */
/*
 * ReplaceCharInTextBuffer
 *
 * The \fIReplaceCharInTextBuffer\fR function is used to replace
 * the character in the TextBuffer \fItext\fR at \fIlocation\fR
 * with the character \fIc\fR.
 *
 * See also:
 *
 * ReplaceBlockInTextBuffer(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern EditResult
ReplaceCharInTextBuffer (TextBuffer *text, TextLocation *location, int c, TextUpdateFunction f, caddr_t d)
{
char buffer[2];
TextLocation nextloc;
EditResult retval;

buffer[0] = c;
buffer[1] = '\0';

LockBuffer();
nextloc = NextLocation(text, *location);
if (nextloc.line == 0 && nextloc.offset == 0)
   nextloc = *location;

retval = ReplaceBlockInTextBuffer(text, location, &nextloc, buffer, f, d);
UnlockBuffer();
return retval;
} /* end of ReplaceCharInTextBuffer */
/*
 * ReplaceBlockInTextBuffer
 *
 * The \fIReplaceBlockInTextBuffer\fR function is used to update the contents
 * of the TextBuffer associated with \fItext\fR.  The characters stored
 * between \fIstartloc\fR and \fIendloc\fR are deleted and the \fIstring\fR
 * is inserted after \fIstartloc\fR.  If the edit succeeds the 
 * TextUpdateFunction \fIf\fR is called with the parameters: \fId\fR, 
 * \fItext\fR, and 1; then any other \fBdifferent\fR update functions
 * associated with the TextBuffer are called with their associated data
 * pointer, \fItext\fR, and 0.
 *
 * This function records the operation performed in TextUndoItem structures.
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
 * ReplaceCharInTextBuffer(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern EditResult
ReplaceBlockInTextBuffer (TextBuffer *text, TextLocation *startloc, TextLocation *endloc, char *string, TextUpdateFunction f, caddr_t d)
{
register int i;
EditResult retval;

LockBuffer();
FreeTextUndoList(text);

if (startloc-> line != endloc-> line || startloc-> offset != endloc-> offset)
   text-> deleted.string = GetTextBufferBlock
      (text, *startloc, PreviousLocation(text, *endloc));
if (string != NULL && *string != '\0')
   text-> insert.string = strcpy(MALLOC(strlen(string) + 1), string);

text-> insert.hint =
text-> deleted.hint = 0;

text-> deleted.start =
text-> insert.start =
text-> insert.end   = *startloc;
text-> deleted.end   = *endloc;

text-> deleted.start.buffer = text-> insert.start.buffer =
	(char *) PositionOfLocation(text, *startloc);
text-> deleted.end.buffer = (char *) PositionOfLocation(text, *endloc);

retval = DeleteBlockInTextBuffer(text, startloc, endloc);

if (retval == EDIT_SUCCESS)
   {
   retval = InsertBlockIntoTextBuffer(text, startloc, string);
   if (retval == EDIT_SUCCESS)
      {
      text-> dirty = 1;
      text-> insert.end = *startloc;
      text-> insert.end.buffer = (char *) PositionOfLocation(text, *startloc);
      for (i = 0; i < text-> refcount; i++)
         if (text-> update[i].f == f && text-> update[i].d == d)
            {
            (*text-> update[i].f) (text-> update[i].d, text, 1);
            break;
            }
      for (i = 0; i < text-> refcount; i++)
         if (text-> update[i].f != f || text-> update[i].d != d)
            (*text-> update[i].f) (text-> update[i].d, text, 0);
      }
   }
else
   retval = EDIT_FAILURE;

UnlockBuffer();
return (retval);

} /* end of ReplaceBlockInTextBuffer */
/*
 * LocationOfPosition
 *
 * The \fILocationOfPosition\fR function is used to translate a 
 * \fIposition\fR in the \fItext\fR TextBuffer to a TextLocation.
 * It returns the translated TextLocation.  If the \fIposition\fR is invalid
 * the Buffer pointer \fIbuffer\fP is set to NULL and the line and 
 * offset members are set the the last valid location in 
 * the TextBuffer; otherwise \fIbuffer\fP is set to a non-NULL 
 * (though useless) value.
 *
 * See also:
 *
 * LineOfPosition(3), PositionOfLocation(3), LocationOfPosition(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextLocation
LocationOfPosition (TextBuffer *text, TextPosition position)
{
TextLocation location;
register TextPage i;
register TextPosition c = (TextPosition)0;
register TextLine l = (TextLine)0;

LockBuffer();
for (i = (TextPage)0; c < position && i < text-> pages.used; i++)
   {
   c += text-> pages.p[i].bytes;
   l += text-> pages.p[i].lines;
   }

if (position == (TextPosition)0)
   {
   location.line = (TextLine)0;
   location.offset = (TextPosition)0;
   location.buffer = "";
   }
else
   if (c <= position && i == text-> pages.used)
      {
      location.line = l - 1;
      location.offset = c - 1;
      location.buffer = NULL;
      }
   else
      {
      i--;
      c -= text-> pages.p[i].bytes;
      l -= text-> pages.p[i].lines;
      while (c <= position)
         c += text-> lines.p[l++].buffer-> used;
      c -= text-> lines.p[--l].buffer-> used;
      location.line   = l;
      location.offset = position - c;
      location.buffer = "";
      }

UnlockBuffer();
return (location);

} /* end of LocationOfPosition */
/*
 * LineOfPosition
 *
 * The \fILineOfPosition\fR function is used to translate a 
 * \fIposition\fR in the \fItext\fR TextBuffer to a line index.
 * It returns the translated line index or EOF if the \fIposition\fR
 * is invalid.
 *
 * See also:
 *
 * LineOfPositon(3), PositionOfLocation(3), LocationOfPosition(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern int
LineOfPosition (TextBuffer *text, TextPosition position)
{
TextLocation location;
int retval;

LockBuffer();
location = LocationOfPosition(text, position);
if (location.buffer == NULL)
   location.line = EOF;

retval = location.line;
UnlockBuffer();
return retval;

} /* end of LineOfPosition */
/*
 * PositionOfLine
 *
 * The \fIPositionOfLine\fR function is used to translate a 
 * \fIlineindex\fR in the \fItext\fR TextBuffer to a TextPosition.
 * It returns the translated TextPosition or EOF if the \fIlineindex\fR
 * is invalid.
 *
 * See also:
 *
 * LineOfPositon(3), PositionOfLocation(3), LocationOfPosition(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextPosition
PositionOfLine (TextBuffer *text, TextLine lineindex)
{
TextPage i;
TextLine j;
TextPosition position = (TextPosition)EOF;

LockBuffer();
if (lineindex >= (TextLine)0 && lineindex < text-> lines.used)
   {
   position = (TextPosition)0;
   for (i = (TextPage)0; i < text-> lines.p[lineindex].pageindex; i++)
      position += text-> pages.p[i].bytes;
   for (j = FirstLineOfPage(text, i); j < lineindex; j++)
      position += text-> lines.p[j].buffer-> used;
   }

UnlockBuffer();
return (position);

} /* end of PositionOfLine */
/*
 * LastTextBufferLocation
 *
 * The \fILastTextBufferLocation\fR function returns the last valid
 * TextLocation in the TextBuffer associated with \fItext\fR.
 *
 * See also:
 *
 * LastTextBufferPosition(3), FirstTextBufferLocation(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextLocation
LastTextBufferLocation (TextBuffer *text)
{
TextLocation last;

LockBuffer();
last.line = LastTextBufferLine(text);
last.offset = LastCharacterInTextBufferLine(text, last.line);

UnlockBuffer();
return (last);

} /* end of LastTextBufferLocation */
/*
 * LastTextBufferPosition
 *
 * The \fILastTextBufferPosition\fR function returns the last valid
 * TextPositon in the TextBuffer associated with \fItext\fR.
 *
 * See also:
 *
 * LastTextBufferLocation(3), FirstTextBufferLocation(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextPosition
LastTextBufferPosition (TextBuffer *text)
{

return (PositionOfLocation(text, LastTextBufferLocation(text)));

} /* end of LastTextBufferPosition */
/*
 * PositionOfLocation
 *
 * The \fIPositionOfLocation\fR function is used to translate a 
 * \fIlocation\fR in the \fItext\fR TextBuffer to a TextPosition.
 * The function returns the translated TextPosition or EOF if 
 * the \fIlocation\fR is invalid.
 *
 * See also:
 *
 * PositionOfLine(3), LocationOfPosition(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextPosition
PositionOfLocation (TextBuffer *text, TextLocation location)
{
TextPosition position = PositionOfLine(text, location.line);

LockBuffer();
if (position != (TextPosition)EOF)
   if (location.offset >= text-> lines.p[location.line].buffer-> used)
      position = (TextPosition)EOF;
   else
      position += location.offset;

UnlockBuffer();
return (position);

} /* end of PostionOfLocation */
/*
 * GetTextBufferLine
 *
 * The \fIGetTextBufferLine\fR function is used to retrieve the contents of
 * \fIline\fR from the \fItext\fR TextBuffer.  It returns a pointer to a 
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
 * GetTextBufferLocation(3), GetTextBufferChar(3), GetTextBufferBlock(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern char *
GetTextBufferLine (TextBuffer *text, TextLine lineindex)
{
register char * p;

LockBuffer();
p = GetTextBufferLocation(text, lineindex, (TextLocation *)NULL);
if (p != NULL)
   p = strcpy(MALLOC((unsigned)(strlen(p) + 1)), p);

UnlockBuffer();
return (p);

} /* end of GetTextBufferLine */
/*
 * GetTextBufferChar
 *
 * The \fIGetTextBufferChar\fR function is used to retrieve a character
 * stored in the \fItext\fR TextBuffer at \fIlocation\fR.  It returns
 * either the character itself or EOF if location is outside the range
 * of valid locations within the TextBuffer.
 *
 * See also:
 *
 * GetTextBufferLocation(3), GetTextBufferBlock(3), GetTextBufferLine(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

int
GetTextBufferChar (TextBuffer *text, TextLocation location)
{
char * p;
register int c = EOF;

LockBuffer();
p = GetTextBufferLocation(text, location.line, (TextLocation *)NULL);
if (p != NULL)
   if (location.offset < (int)strlen(p))   /* ANSI_C */
      c = p[location.offset];

UnlockBuffer();
return (c == '\0' ? '\n' : c);

} /* end of GetTextBufferChar */
/*
 * CopyTextBufferBlock
 *
 * The \fICopyTextBufferBlock\fR function is used to retrieve a text block
 * from the \fItext\fR TextBuffer.  The block is defined as the characters
 * between \fIstart_position\fR and \fIend_position\fR inclusive.  It
 * returns the number of bytes copied; if the parameters are invalid
 * the return value is zero (0).  
 *
 * Note:
 *
 * The storage for the copy is allocated by the caller.  It is the 
 * responsibility of the caller to ensure that enough storage is allocated
 * to copy end_position - start_position + 1 bytes.
 *
 * See also:
 *
 * GetTextBufferLocation(3), GetTextBufferChar(3), GetTextBufferLine(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern int
CopyTextBufferBlock (TextBuffer *text, char *buffer, TextPosition start_position, TextPosition end_position)
{
int retval;
TextLocation start_location;
TextLocation end_location;
char * p = buffer;

LockBuffer();
start_location = LocationOfPosition(text, start_position);
end_location = LocationOfPosition(text, end_position);

if (end_position < start_position ||
    start_location.buffer == NULL || end_location.buffer == NULL)
   retval = 0;
else
   {
   TextPosition size      = end_position - start_position + 1;
   TextLine     lineindex = start_location.line;
   TextPosition inpos     = start_location.offset;
   TextPosition outpos    = 0;
   char * t = GetTextBufferLocation(text, start_location.line,(TextLocation *)NULL);
   while (outpos < size)
      {
      switch(t[inpos])
         {
         case '\0':
            p[outpos++] = '\n';
            t = GetTextBufferLocation(text, ++lineindex, (TextLocation *)NULL);
            inpos = (TextPosition)0;
            break;
         default:
            p[outpos++] = t[inpos++];
            break;
         }
      }
   retval = size;
   p[outpos] = '\0';
   }

UnlockBuffer();
return (retval);

} /* end of CopyTextBufferBlock */
/*
 * GetTextBufferBlock
 *
 * The \fIGetTextBufferBlock\fR function is used to retrieve a text block
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
 * GetTextBufferLocation(3), GetTextBufferChar(3), GetTextBufferLine(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern char *
GetTextBufferBlock (TextBuffer *text, TextLocation start_location, TextLocation end_location)
{
char * p  = NULL;
TextPosition start_position;
TextPosition end_position;

LockBuffer();
start_position = PositionOfLocation(text, start_location);
end_position   = SameTextLocation(start_location, end_location) ?
   start_position : PositionOfLocation(text, end_location);
if (start_position != (TextPosition)EOF && end_position != (TextPosition)EOF &&
    start_position <= end_position)
   {
   TextPosition size      = end_position - start_position + 1;
   TextLine     lineindex = start_location.line;
   TextPosition inpos     = start_location.offset;
   TextPosition outpos    = 0;
   char * t = GetTextBufferLocation(text, start_location.line,(TextLocation *)NULL);

   p = CALLOC((unsigned)size + 1, sizeof(char));
   while (outpos < size)
      {
      switch(t[inpos])
         {
         case '\0':
            p[outpos++] = '\n';
            t = GetTextBufferLocation(text, ++lineindex, (TextLocation *)NULL);
            inpos = (TextPosition)0;
            break;
         default:
            p[outpos++] = t[inpos++];
            break;
         }
      }
   p[outpos] = '\0';
   }

UnlockBuffer();
return (p);

} /* end of GetTextBufferBlock */
/*
 * SaveTextBuffer
 *
 * The \fISaveTextBuffer\fR function is used to write the contents
 * of the \fItext\fR TextBuffer to the file \fIfilename\fR.  It returns
 * a SaveResult which can be:~
 *
 * .so CWstart
 *    SAVE_FAILURE
 *    SAVE_SUCCESS
 * .so CWend
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern SaveResult
SaveTextBuffer (TextBuffer *text, char *filename)
{
FILE * fp;
register TextLine i;

/* NOTE: NEED TO CHECK IF FILE CAN BE WRITTEN, ETC */

LockBuffer();
if (filename == NULL)
   filename = text-> filename;
else
   {
   if (text-> filename != NULL)
      FREE(text-> filename);
   text-> filename = strcpy(MALLOC((unsigned)(strlen(filename) + 1)), filename);
   }

if (filename != NULL && (fp = fopen(filename, "w")) != NULL)
   {
   if (!TextBufferEmpty(text))
      for (i = (TextLine)0; i < text-> lines.used; i++)
         (void) fprintf(fp, "%s\n", 
                        GetTextBufferLocation(text, i, (TextLocation *)NULL));
   (void) fclose(fp);
   text-> dirty = 0;
   UnlockBuffer();
   return (SAVE_SUCCESS);
   }
else {
   UnlockBuffer();
   return (SAVE_FAILURE);
}

} /* end of SaveTextBuffer */
/*
 * GetTextBufferBuffer
 *
 * The \fIGetTextBufferBuffer\fR function is used to retrieve a pointer to the
 * Buffer stored in TextBuffer \fItext\fR for \fIline\fR.  This pointer
 * is volatile; subsequent calls to any TextBuffer routine may make it
 * invalid.  If a more permanent copy of this Buffer is required the Buffer
 * Utility CopyBuffer can be used to create a private copy of it.
 *
 * See also:
 *
 * GetTextBufferBlock(3), GetTextBufferLocation(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern Buffer *
GetTextBufferBuffer (TextBuffer *text, TextLine line)
{
char *p;
Buffer *retval;

LockBuffer();
p = GetTextBufferLocation(text, line, (TextLocation *)NULL);
retval = (p == NULL ? (Buffer *)NULL : text-> buffer);
UnlockBuffer();
return retval;
} /* end of GetTextBufferBuffer */
/*
 * IncrementTextBufferLocation
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
 * NextLocation(3), PreviousLocation(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextLocation
IncrementTextBufferLocation (TextBuffer *text, TextLocation location, TextLine line, TextPosition offset)
{
TextLocation newlocation;

LockBuffer();
if (location.line >= 0 && location.line < text-> lines.used &&
   location.offset >= 0 &&
   location.offset < text-> lines.p[location.line].buffer-> used)
   {
   if (line == 0 && offset == 0)
      newlocation = location;
   else
      if (line == 0 && offset != 0)
         {
         TextPosition new_pos = PositionOfLocation(text, location) + offset;
         if (new_pos >= 0)
            {
            newlocation = LocationOfPosition(text, new_pos);
            if (newlocation.buffer == NULL)
               newlocation = location;
            }
         else
            newlocation = location;
         }
      else
         {
         newlocation.line = location.line + line;
         if (newlocation.line >= 0 && newlocation.line < text-> lines.used)
            {
            newlocation.offset = offset == 0 ?
               location.offset : offset;
            if (newlocation.offset >=
                text-> lines.p[newlocation.line].buffer-> used)
               newlocation.offset =
                  text-> lines.p[newlocation.line].buffer-> used - 1;
            }
         else
            newlocation = location;
         }
   }
else
   newlocation = location;

UnlockBuffer();
return (newlocation);

} /* end of IncrementTextBufferLocation */
/*
 * PreviousLocation
 *
 * The \fIPreviousLocation\fR function returns the Location which precedes
 * the given \fIcurrent\fR location in a TextBuffer.  If the current
 * location points to the beginning of the TextBuffer this function wraps.
 *
 * See also:
 *
 * NextLocation(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextLocation
PreviousLocation (TextBuffer *textBuffer, TextLocation current)
{

LockBuffer();
if (--current.offset < 0)
   {
   if (--current.line < 0)
      current.line = LastTextBufferLine(textBuffer);
   current.offset = LastCharacterInTextBufferLine(textBuffer, current.line);
   }

UnlockBuffer();
return (current);

} /* end of PreviousLocation */
/*
 * NextLocation
 *
 * The \fINextLocation\fR function returns the TextLocation which follows
 * the given \fIcurrent\fR location in a TextBuffer.  If the current
 * location points to the end of the TextBuffer this function wraps.
 *
 * See also:
 *
 * PreviousLocation(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextLocation
NextLocation (TextBuffer *textBuffer, TextLocation current)
{

LockBuffer();
if (++current.offset > LastCharacterInTextBufferLine(textBuffer, current.line))
   {
   if (++current.line > LastTextBufferLine(textBuffer))
      current.line = 0;
   current.offset = 0;
   }

UnlockBuffer();
return (current);

} /* end of NextLocation */

/*
 * The _NextLocationWithoutWrap function returns the TextLocation which follows 
 * the given current location in a TextBuffer.If we go beyond the buffer,
 * don't wrap
 */

extern TextLocation
_NextLocationWithoutWrap (TextBuffer *textBuffer, TextLocation current)
{

if (++current.offset > LastCharacterInTextBufferLine(textBuffer, current.line))   {
   if (++current.line > LastTextBufferLine(textBuffer))
      current.line = LastTextBufferLine(textBuffer);
   current.offset = LastCharacterInTextBufferLine(textBuffer, current.line);
   }

return (current);

}
 
/*
 * StartCurrentTextBufferWord
 *
 * The \fIStartCurrentTextBufferWord\fR function is used to locate the beginning
 * of a word in the TextBuffer relative to a given \fIcurrent\fR location.
 * The function returns the location of the beginning of the current
 * word.  Note: this return value will equal the given current value
 * if the current location is the beginning of a word.
 *
 * See also:
 *
 * PreviousTextBufferWord(3), NextTextBufferWord(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextLocation
StartCurrentTextBufferWord (TextBuffer *textBuffer, TextLocation current)
{
char * p;
TextLocation retval;

LockBuffer();
p = GetTextBufferLocation(textBuffer, current.line, NULL);
if (IS_A_WORD_CHAR(p[current.offset]))
   {
   while(current.offset >= 0 && IS_A_WORD_CHAR(p[current.offset]))
      current.offset--;
   }
else
   {
   while(current.offset >= 0 && IS_NOT_A_WORD_CHAR(p[current.offset]))
      current.offset--;
   while(current.offset >= 0 && IS_A_WORD_CHAR(p[current.offset]))
      current.offset--;
   }

current.offset++;

#ifndef DEBUG_TEXTBUFF

UnlockBuffer();
return (current);
#else
current = NextLocation(textBuffer, current);
(void) fprintf(stderr,
	       "in a CurrentText...%d.%d\n", current.line, current.offset);

retval = PreviousTextBufferWord(textBuffer, current);
UnlockBuffer();
return retval;
#endif

} /* end of StartCurrentTextBufferWord */
/*
 * EndCurrentTextBufferWord
 *
 * The \fIEndCurrentTextBufferWord\fR function is used to locate the end
 * of a word in the TextBuffer relative to a given \fIcurrent\fR location.
 * The function returns the location of the end of the current
 * word.  Note: this return value will equal the given currrent value
 * if the current location is already at the end of a word.
 *
 * See also:
 *
 * PreviousTextBufferWord(3), NextTextBufferWord(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextLocation
EndCurrentTextBufferWord (TextBuffer *textBuffer, TextLocation current)
{
char * p;
int end;

LockBuffer();
p = GetTextBufferLocation(textBuffer, current.line, NULL);
end = LastCharacterInTextBufferLine(textBuffer,current.line);
if (IS_A_WORD_CHAR(p[current.offset]))
   {
   while(current.offset < end && IS_A_WORD_CHAR(p[current.offset]))
      current.offset++;
   }
else
   {
   while(current.offset < end && IS_NOT_A_WORD_CHAR(p[current.offset]))
      current.offset++;
   while(current.offset < end && IS_A_WORD_CHAR(p[current.offset]))
      current.offset++;
   }

UnlockBuffer();
return (current);

} /* end of EndCurrentTextBufferWord */
/*
 * PreviousTextBufferWord
 *
 * The \fIPreviousTextBufferWord\fR function is used to locate the
 * beginning of a word in a TextBuffer relative to a given \fIcurrent\fR
 * location.  It returns the location of the beginning of the word 
 * which precedes the given current location.  If the current location
 * is within a word this function will skip over the current word.
 * If the current word is the first word in the TextBuffer the function
 * wraps to the end of the buffer.
 *
 * See also:
 *
 * PreviousTextBufferWord(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextLocation
PreviousTextBufferWord (TextBuffer *textBuffer, TextLocation current)
{
TextLocation new;
TextLocation save;
register int i;
int done;
char * p;
TextLocation retval;

LockBuffer();
save = current;
new = PreviousLocation(textBuffer, current);

p = GetTextBufferLocation(textBuffer, new.line, NULL);

if (IS_NOT_A_WORD_CHAR(p[new.offset]))
   {
   current = PreviousLocation(textBuffer, new);
   if (new.line != current.line)
      p = GetTextBufferLocation(textBuffer, current.line, NULL);

   for (done = 0; done == 0; current = new)
      {

      /* avoid endless loop */
      if (save.line == current.line && save.offset == current.offset) {
	 UnlockBuffer();
         return(current);
      }
      if (IS_A_WORD_CHAR(p[current.offset]))
         done = 1;
      new = PreviousLocation(textBuffer, current);
      if (new.line != current.line)
         p = GetTextBufferLocation(textBuffer, new.line, NULL);
      }
   }

for (current = new, done = 0;
     done == 0;
     current = new)
   {

   /* avoid endless loop */
   if (save.line == current.line && save.offset == current.offset) {
      UnlockBuffer();
      return(current);
   }

#ifdef DEBUG_TEXTBUFF
   (void) fprintf(stderr,"trying %d.%d %d\n",
           current.line, current.offset, p[current.offset]);
#endif
   if (IS_NOT_A_WORD_CHAR(p[current.offset]))
      done = 1;
   else
      {
      new = PreviousLocation(textBuffer, current);
      if (new.line != current.line)
         p = GetTextBufferLocation(textBuffer, new.line, NULL);
      }
   }

retval =  NextLocation(textBuffer, current);
UnlockBuffer();
return retval;

} /* end of PreviousTextBufferWord */
/*
 * NextTextBufferWord
 *
 * The \fINextTextBufferWord\fR function is used to locate the beginning
 * of the next word from a given \fIcurrent\fR location in a TextBuffer.
 * If the current location is within the last word in the TextBuffer
 * the function wraps to the beginning of the TextBuffer.
 *
 * See also:
 *
 * PreviousTextBufferWord(3), StartCurrentTextBufferWord(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextLocation
NextTextBufferWord (TextBuffer *textBuffer, TextLocation current)
{
TextLocation new;
register int i;
int phase;
int l;
char * p;

LockBuffer();
new = current;
phase = 0;
do
   {
   if (!(p = GetTextBufferLocation(textBuffer, new.line, NULL)))
      break; /* @ error! */
   else
      {
      l = strlen(p);
      for (i = new.offset; i < l && phase != 2; i++)
         if (IS_NOT_A_WORD_CHAR(p[i]))
            phase = 1;
         else
            if (phase == 1)
               phase = 2;
      if (phase == 2)
         new.offset = i - 1;
      else
         {
         phase = 1;
         if (++new.line > LastTextBufferLine(textBuffer))
            new.line = 0;
         new.offset = 0;
         }
      }
   } while (phase != 2 && new.line != current.line);

UnlockBuffer();
return (new);

} /* end of NextTextBufferWord */
/*
 * RegisterTextBufferUpdate
 *
 * The \fIRegisterTextBufferUpdate\fR procedure associates the 
 * TextUpdateFunction \fIf\fR and data pointer \fId\fR with the
 * given TextBuffer \fItext\fR.  
 * This update function will be called whenever an update operation
 * is performed on the TextBuffer.  See ReplaceBlockInTextBuffer
 * for more details.
 *
 * Note:
 *
 * Calling this function increments a reference count mechanism 
 * used to determine when to actually free the TextBuffer.  Calling 
 * the function with a NULL value for the function circumvents 
 * this mechanism.
 *
 * See also:
 * 
 * UnregisterTextBufferUpdate(3), ReadStringIntoTextBuffer(3),
 * ReadFileIntoTextBuffer(3)
 *
 * Synopsis:
 *
 * #include<textbuff.h>
 *  ...
 */

extern void
RegisterTextBufferUpdate (TextBuffer *text, TextUpdateFunction f, caddr_t d)
{
register int i;

LockBuffer();
if (f != NULL)
   {
   i = text-> refcount++;

   text-> update = (TextUpdateCallback *)
      REALLOC(((char *)
		text-> update), text-> refcount * sizeof(TextUpdateCallback));

   text-> update[i].f = f;
   text-> update[i].d = d;
   }

UnlockBuffer();
} /* end of RegisterTextBufferUpdate */
/*
 * UnregisterTextBufferUpdate
 *
 * The \fIUnregisterTextBufferUpdate\fR function disassociates the 
 * TextUpdateFunction \fIf\fR and data pointer \fId\fR with the
 * given TextBuffer \fItext\fR.  If the function/data pointer pair
 * is not associated with the given TextBuffer zero is returned
 * otherwise the association is dissolved and one is returned.
 *
 * See also:
 * 
 * RegisterTextBufferUpdate(3), FreeTextBuffer(3)
 *
 * Synopsis:
 *
 * #include<textbuff.h>
 *  ...
 */

extern int
UnregisterTextBufferUpdate (TextBuffer *text, TextUpdateFunction f, caddr_t d)
{
register int i;
int retval;

LockBuffer();
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
      FREE(((char *)text-> update));
      text-> update = (TextUpdateCallback *)NULL;
      }
   else
      {
      if (i < text-> refcount)
         memmove(&text-> update[i],
                 &text-> update[i+1],
                 (text-> refcount - i) * sizeof(TextUpdateCallback));
      text-> update = (TextUpdateCallback *)
         REALLOC(((char *)
		text-> update), text-> refcount * sizeof(TextUpdateCallback));
      }
   retval = 1;
   }

UnlockBuffer();
return (retval);

} /* end of UnregisterTextBufferUpdate */
/*
 * InitTextUndoList
 *
 */

static void
InitTextUndoList (TextBuffer *text)
{

text-> insert.string =
text-> deleted.string = NULL;

} /* end of InitTextUndoList */
/*
 * FreeTextUndoList
 *
 */

static void
FreeTextUndoList (TextBuffer *text)
{

if (text-> insert.string != NULL)
   FREE(text-> insert.string);

if (text-> deleted.string != NULL)
   FREE(text-> deleted.string);

InitTextUndoList(text);

} /* end of FreeTextUndoList */


/*
 * RegisterTextBufferScanFunctions
 *
 * The \fIRegisterTextBufferScanFunctions\fR procedure provides the capability
 * to replace the scan functions used by the ForwardScanTextBuffer and
 * BackwardScanTextBuffer functions.  These functions are called as:~
 *
 * .so CWstart
 * 	(*forward_scan_func)(string, curp, expression);
 * 	(*backward_scan_func)(string, curp, expression);
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
 * #include <textbuff.h>
 *  ...
 */
void
RegisterTextBufferScanFunctions(OlStrScanDefProc forward_scan_func,
	OlStrScanDefProc backward_scan_func)
{

	LockBuffer();
	Strexp  = forward_scan_func ?
		forward_scan_func  : (OlStrScanDefProc)strexp;

	Strrexp = backward_scan_func ?
		backward_scan_func : (OlStrScanDefProc)strrexp;
	UnlockBuffer();
}


/*
 * RegisterTextBufferWordDefinition
 *
 * The \fIRegisterTextBufferWordDefinition\fR procedure provides the 
 * capability to replace the word definition function used by the 
 * TextBuffer Utilities.  This function is called as:~
 *
 * .so CWstart
 * 	(*word_definition)(c);
 * .so CWend
 *
 * The function is responsible for returning non-zero if the character
 * c is considered a character that can occur in a word and zero
 * otherwise.
 *
 * Calling this function with NULL reinstates the default word definition
 * which allows the following set of characters: a-zA-Z0-9_
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

void
RegisterTextBufferWordDefinition(
	OlStrWordDefProc word_definition_func)
{

	LockBuffer();
	IsAWordChar = word_definition_func ?
		word_definition_func : (OlStrWordDefProc)is_a_word_char;
	UnlockBuffer();

}


/*
 * is_a_word_char
 *
 */
static int
is_a_word_char (int rc)
{
unsigned char c = (unsigned char)rc;
int retval = 
   ('a' <= c && c <= 'z') || 
   ('A' <= c && c <= 'Z') || 
   ('0' <= c && c <= '9') ||
   (c == '_')		  ||
   (192 <= (unsigned int)c && (unsigned int)c <= 255);

return (retval);

} /* end of is_a_word_char */
