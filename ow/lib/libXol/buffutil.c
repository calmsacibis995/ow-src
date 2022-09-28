#pragma ident	"@(#)buffutil.c	302.5	97/03/26 lib/libXol SMI"	/* olmisc:buffutil.c 1.2	*/

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

/*
 * buffutil.c
 */

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/buffutil.h>
#include <Xol/memutil.h>

/*
 * AllocateBuffer
 *
 * The \fIAllocateBuffer\fR function allocates a Buffer for elements
 * of the given \fIelement_size\fR.
 * The used member of the Buffer is set to zero and
 * the size member is set to the value of \fIinitial_size\fR.
 * If \fIinitial_size\fR is zero the pointer p is set to NULL, otherwise
 * the amount of space required (\fIinitial_size\fR * \fIelement_size\fR)
 * is allocated and the pointer p is set to point to this space.
 * The function returns the pointer to the allocated Buffer.
 *
 * Private:
 *
 * This function is used to allocate a new Buffer.  The algorithm is -
 *
 *  1. Allocate a buffer structure.
 *  2. Set the element size to the size specified by the caller.
 *  3. Set the size and used elements in this structure to zero(0).
 *  4. Set the pointer to NULL.
 *  5. If the caller specified an initial size (which is a number of
 *     elements) then call GrowBuffer to expand the buffer to the
 *     specified extent.
 *
 * See also:
 *
 * FreeBuffer(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *  ...
 */

extern Buffer *
AllocateBuffer (int element_size, int initial_size)
{
Buffer * b = (Buffer *) MALLOC(sizeof(Buffer));

b-> esize = element_size;
b-> size = b-> used = 0;
b-> p = NULL;

if (initial_size != 0)
   GrowBuffer(b, initial_size);

return (b);

} /* end of AllocateBuffer */
/*
 * GrowBuffer
 *
 * The \fIGrowBuffer\fR procedure is used to expand (or compress) a 
 * given \fIbuffer\fR size by \fIincrement\fR elements.  If the increment
 * is negative the operation results in a reduction in the size
 * of the Buffer.
 *
 * Private:
 *
 * This procedure is used to extend a given buffer by a given number
 * of elements.  It assumes that the element size is stored in the
 * buffer and uses this value to calculate the amount of space required.
 * The algorithm is -
 *
 *  1. Increment the buffer size by the specified value.
 *  2. Allocate (or reallocate) storage for the buffer pointer.
 *
 * See also:
 *
 * AllocateBuffer(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *  ...
 */

extern void
GrowBuffer (Buffer *b, int increment)
{
b-> size += increment;

if (b-> size == increment)
   b-> p = (BufferElement *)MALLOC((unsigned)(b-> size * b-> esize));
else
   b-> p = (BufferElement *)REALLOC(b-> p, (unsigned)(b-> size * b-> esize));

} /* end of GrowBuffer */
/*
 * CopyBuffer
 *
 * The \fICopyBuffer\fR function is used to allocate a new
 * Buffer with the same attributes as the given \fIbuffer\fR and to copy
 * the data associated with the given \fIbuffer\fR into the new Buffer.
 * A pointer to the newly allocated and initialized Buffer is returned.
 *
 * Private:
 *
 * This function creates a copy of a given buffer and returns the
 * pointer to the new buffer to the caller.  That is, this routine
 * is used to "clone" a buffer.
 *
 * See also:
 *
 * AllocateBuffer(3), FreeBuffer(3), InsertIntoBuffer(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *  ...
 */

extern Buffer *
CopyBuffer (Buffer *buffer)
{
Buffer * newbuffer = AllocateBuffer(buffer-> esize, buffer-> size);

newbuffer-> used = buffer-> used;

/* The following if is a hack to accomodate the case of wide_char
   char position buffers used in Mltextbuff.c which have a size
   0 but there buffer->used reflects chars in line. Suffice to say
   it doesn't effect the noraml behavior of this function */

if(newbuffer->size > 0)
(void) memcpy(newbuffer-> p, buffer-> p, buffer-> used * buffer-> esize);

return (newbuffer);

} /* end of CopyBuffer */
/*
 * FreeBuffer
 *
 * The \fIFreeBuffer\fR procedure is used to deallocate (free)
 * storage associated with the given \fIbuffer\fR pointer.
 *
 * Private:
 *
 * This procedure is used to free the storage associate with a Buffer.
 * It simply frees the storage in the buffer and the buffer itself.
 * It checks the pointer in the buffer (since AllocateBuffer sets
 * it to NULL and the caller may never have called GrowBuffer to
 * allocate space within the buffer).  It presumes that the given
 * buffer pointer itself is valid, however.
 *
 * See also:
 *
 * AllocateBuffer(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *  ...
 */

extern void
FreeBuffer (Buffer *buffer)
{

if (buffer-> p)
   FREE(buffer-> p);
FREE((char *)buffer);

} /* end of FreeBuffer */
/*
 * InsertIntoBuffer
 *
 * The \fIInsertIntoBuffer\fR function is used to insert the
 * elements stored in the \fIsource\fR buffer into the \fItarget\fR
 * buffer \fIbefore\fR the element stored at \fIoffset\fR.
 * If the \fIoffset\fR is invalid or if the \fIsource\fR buffer is
 * empty the function returns zero otherwise it returns one after
 * completing the insertion.
 *
 * Private:
 *
 * This function is used to insert a source buffer into a target buffer
 * at a specified offset.  The algorithm used is -
 *
 *  1. Check the offset for range in the target and is source used for
 *     greater than zero(0).  If the chaeck fails return failure.
 *  2. Calculate the amount of space needed above what is available
 *     in the target.  If more space is needed grow the target buffer
 *     by this amount PLUS the LNINCRE amount (to avoid trashing).
 *  3. Check if the operation is a simple append (and if so append the
 *     source to the target, otherwise shift the target (so that the
 *     source will fit) and then copy the source into the target.
 *  4. Return success.
 *
 * See also:
 *
 * ReadStringIntoBuffer(3), ReadFileIntoBuffer(3), BufferMacros(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *  ...
 */

extern int
InsertIntoBuffer (Buffer *target, Buffer *source, int offset)
{
if (offset < 0 || offset > target-> used || source-> used <= 0)
   return (0);
else
   {
   int space_needed = source-> used - (target-> size - target-> used);
#ifdef DEBUG
fprintf(stderr, "SRC = %d(%d) '%s' DST = %d(%d) '%s'\n",
source-> used, source-> size, source-> p,
target-> used, target-> size, target-> p);
#endif
   if (space_needed > 0)
      GrowBuffer(target, space_needed + LNINCRE);

   if (offset != target-> used)
      memmove(&target-> p[(offset + source-> used - 1) * target-> esize],
              &target-> p[offset * target-> esize],
              (target-> used - offset) * target-> esize);

   memmove(&target-> p[offset * target-> esize], source-> p,
           (source-> used - 1) * target-> esize);
   target-> used += (source-> used - 1);
#ifdef DEBUG
fprintf(stderr, "SRC = %d(%d) '%s' DST = %d(%d) '%s'\n",
source-> used, source-> size, source-> p,
target-> used, target-> size, target-> p);
#endif

   return (1);
   }
} /* end of InsertIntoBuffer */
/*
 * stropen
 *
 * The \fIstropen\fR function copies the \fIstring\fR into a
 * newly allocated Buffer.  This string buffer can be \fIread\fR using the
 * \fIstrgetc\fR function and \fIclosed\fR using the \fIstrclose\fR procedure.
 *
 * Private:
 *
 * This function "opens" a string for "reading".  It basically
 * "bufferizes" a given string.  This buffer can be intelligently
 * accessed using the strgetc function and "closed" using the strclose
 * procedure.  The algorithm is -
 *
 *  1. If the string pointer passed in is NULL then set it to a NULL STRING.
 *  2. Allocate a buffer large enough to accommodate the string.
 *  3. Copy the string into the buffer.
 *  4. Decrement size (to avoid permitting reading past the end
 *     of the string).
 *  5. Return the buffer pointer.
 *
 * See also:
 *
 * strclose(3), strgetc(3)
 *
 * Synopsis:
 *
 *#include <buffuti.h>
 *  ...
 */

extern Buffer *
stropen (char *string)
{
Buffer * sp = NULL;
register int l;

if (string == NULL)
   string = "";

l = strlen(string) + 1;
sp = AllocateBuffer(sizeof(sp->p[0]), l);
for (--l; l >= 0; l--)
   sp-> p[l] = string[l];
sp-> size--;

return (sp);

} /* end of stropen */
/*
 * strgetc
 *
 * The \fIstrgetc\fR function is used to read the next character
 * stored in the string \fIbuffer\fR.
 * The function returns the next character in the Buffer.
 * When no characters remain the routine returns EOF.
 *
 * Private:
 *
 * This function returns the next character "read" from a string
 * buffer "opened" using stropen.  It returns EOF when the string is
 * exhaused (and it is an error to continue reading once the end is
 * reached).  The algorithm is -
 *
 *  1. If the used counter has reached the size (BufferFilled) then
 *     set c to EOF else set c to the next character in the buffer
 *     and increment the used counter.
 *  2. Return c.
 *
 * See also:
 *
 * stropen(3), strclose(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *  ...
 */

extern int
strgetc (Buffer *sp)
{
int c;

if (BufferFilled(sp))
   c = EOF;
else
   c = (unsigned char)sp-> p[sp-> used++];

return (c);

} /* end of strgetc */
/*
 * strclose
 *
 * The \fIstrclose\fR procedure is used to close a string Buffer
 * which was opened using the \fIstropen\fR function.
 *
 * Private:
 *
 * This procedure "closes" a string "opened" using stropen.  Note: it
 * simply calls FreeBuffer to perform the necessary frees and can
 * be repalced by a macro definition.
 *
 * See also:
 *
 * stropen(3), strgetc(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *  ...
 */

extern void
strclose (Buffer *sp)
{

FreeBuffer(sp);

} /* end of strclose */
/*
 * ReadStringIntoBuffer
 *
 * The \fIReadStringIntoBuffer\fR function reads the buffer associated with
 * \fIsp\fR and inserts the characters read into \fIbuffer\fR.
 * The read operation terminates when either EOF is returned when
 * reading the buffer or when a NEWLINE is encountered.  The function
 * returns the last character read to the caller (either EOF or NEWLINE).
 *
 * Private:
 *
 * This function "reads" an "opened" buffer into a given buffer.  It
 * copies characters from the input into the buffer until either EOF
 * or a NEWLINE is reached.  It performs any necessary expansion of the
 * output buffer during processing and returns the character which
 * caused the operation to cease (EOF or '\n').  This return value
 * can be used by the caller to determine when to stop reading the
 * input (and close the buffer).
 *
 * See also:
 *
 * ReadFileIntoBuffer(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *  ...
 */

extern int
ReadStringIntoBuffer (Buffer *sp, Buffer *buffer)
{
int c;

buffer-> used = 0;

for(;;)
   {
   c = strgetc(sp);
   if (BufferFilled(buffer))
      GrowBuffer(buffer, LNINCRE);
   if (c == '\n' || c == EOF)
      {
      buffer-> p[buffer-> used++] = '\0';
      break;
      }
   else
      buffer-> p[buffer-> used++] = c;
   }


return (c);

} /* end of ReadStringIntoBuffer */
/*
 * ReadFileIntoBuffer
 *
 * The \fIReadFileIntoBuffer\fR function reads the file associated with
 * \fIfp\fR and inserts the characters read into the \fIbuffer\fR.
 * The read operation terminates when either EOF is returned when
 * reading the file or when a NEWLINE is encountered.  The function
 * returns the last character read to the caller (either EOF or NEWLINE).
 *
 * Private:
 *
 * This function reads an opened file into a given buffer.  It
 * copies characters from the input into the buffer until either EOF
 * or a NEWLINE is reached.  It performs any necessary expansion of the
 * output buffer during processing and returns the character which
 * caused the operation to cease (EOF or '\n').  This return value
 * can be used by the caller to determine when to stop reading the
 * input (and close the file).
 *
 * See also:
 *
 * ReadStringIntoBuffer(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *  ...
 */

extern int
ReadFileIntoBuffer (FILE *fp, Buffer *buffer)
{
int c;

buffer-> used = 0;

for (;;)
   {
   if (BufferFilled(buffer))
      GrowBuffer(buffer, LNINCRE);
   if ((c = fgetc(fp)) == '\n' || c == EOF)
      {
      buffer-> p[buffer-> used++] = '\0';
      return c;
      }
   else
      buffer-> p[buffer-> used++] = c;
   }

} /* end of ReadFileIntoBuffer */
