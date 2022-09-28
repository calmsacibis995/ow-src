#pragma ident	"@(#)memutil.c	302.3	97/03/26 lib/libXol SMI"	/* olmisc:memutil.c 1.2	*/

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


#include <Xol/memutil.h>

#include <X11/Intrinsic.h>


#include <stdio.h>
#include <stdlib.h>


#ifdef	DEBUG
	#define QPRINTF(x)  if (debugging) (void) fprintf x
#else
	#define QPRINTF(x)
#endif	/* DEBUG */


static char   debugging;


/*
 * _SetMemutilDebug
 *
 * The \fI_SetMemutilDebug\fR procedure is used to turn the debugging
 * mode of the memutil(3) function set to the logical state of \fIflag\fR.
 * When the debugging mode is on, debugging information is set to stderr
 * indicating the address which is being freed or allocated, the file
 * and line which initiated the memory request.  This output can be analyzed
 * using the checkmem utility) to find memory leaks and inappropriate
 * free requests.
 *
 * See also:
 *
 * _Free(3), _Calloc(3), _Malloc(3), _Realloc(3)
 *
 * Synopsis:
 *
 *#include <memutil.h>
 * ...
 */

extern void _SetMemutilDebug(int flag)
{

debugging = (char)flag;

} /* end of _SetMemutilDebug */
/*
 * _Free
 *
 * The \fI_Free\fR procedure is an enhanced version of free(3).  It 
 * is normally used with the FREE macro.  It ensures that any attempts to free
 * storage using a NULL pointer are reported.  It can also be used
 * as a debugging aid to report any FREE requests (including the file
 * and line where the request originated).
 *
 * Note: A utility \fIcheckmem\fR can be used to analyze the memory
 *       management activity reported by the memutil function set.
 *
 * See also:
 *
 * _Calloc(3), _Malloc(3), _Realloc(3)
 *
 * Synopsis:
 *
 *#include <memutil.h>
 * ...
 */

extern void _Free(char *ptr, char *file, int line)
{

if (ptr)
   {
   XtFree(ptr);
   QPRINTF((stderr,"%x Freeing storage in %s at %d.\n", ptr, file, line));
   }
else
   {
   (void) fprintf(stderr,"freeing storage-> NULL in %s at %d!!!\n", file, line);
   abort();
   /* NOTREACHED */
   }

} /* end of _Free */
/*
 * _Calloc
 *
 * The \fI_Calloc\fR function is an enhanced version of calloc(3).  It 
 * is normally used with the macro CALLOC and is used to verify the success of
 * allocating storage; reporting the file and line that results in
 * a failure to allocate a given size.
 * 
 * It can also be used to debug code to show when and how much space is
 * being allocated.
 *
 * Return Value:
 *
 * Pointer to the allocated space.
 *
 * See also:
 *
 * _Free(3), _Malloc(3), _Realloc(3)
 *
 * Synopsis:
 *
 *#include <memutil.h>
 * ...
 */

extern char * _Calloc(unsigned int n, unsigned int size, char *file, int line)
{
char * p = (char *) XtCalloc(n, size);

if (p == NULL)
   {
   (void) fprintf
      (stderr,"couldn't XtCalloc %d elements of size %d in %s at %d!!!\n", 
       n, size, file, line);
   abort();
   /* NOTREACHED */
   } 
else
   QPRINTF((stderr,"%x XtCalloc %d elements of size %d in %s at %d\n", 
           p, n, size, file, line));

return (p);
   
} /* end of _Calloc */


/*
 * _Malloc
 *
 * The \fI_Malloc\fR function is an enhanced version of malloc(3).  
 * It is normally used with the macro MALLOC and is used to verify the 
 * success of allocating storage; reporting the file and line that results in
 * a failure to allocate a given size.
 * 
 * It can also be used to debug code to show when and how much space is
 * being allocated.
 *
 * Return value:
 *
 * Pointer to the allocated space.
 *
 * See also:
 *
 * _Free(3), _Calloc(3), _Realloc(3)
 *
 * Synopsis:
 *
 *#include <memutil.h>
 * ...
 */

extern char * _Malloc(unsigned int size, char *file, int line)
{
char * p = XtMalloc(size);

if (p == NULL)
   {
   (void) fprintf
      (stderr, "couldn't XtMalloc %d bytes in %s at %d!!!\n", size, file, line);
   abort();
   /* NOTREACHED */
   } 
else
   QPRINTF((stderr,"%x XtMalloc %d bytes in %s at %d\n", p, size, file, line));

return (p);
   
} /* end of _Malloc */
/*
 * _Realloc
 *
 * The \fI_Realloc\fR function is an enhanced version of realloc(3).  
 * It is normally used with the macro REALLOC and is used to verify 
 * the success of allocating storage; reporting the file and line that 
 * results in a failure to allocate a given size.
 * 
 * It can also be used to debug code to show when and how much space is
 * being allocate.
 *
 * Return value:
 *
 * Pointer to the allocated space.
 *
 * See also:
 *
 * _Free(3), _Calloc(3), _Malloc(3)
 *
 * Synopsis:
 *
 *#include <memutil.h>
 * ...
 */

extern char * _Realloc(char *ptr, unsigned int size, char *file, int line)
{

char * p;

if (ptr == NULL)
   p = (char *) _Malloc(size, file, line);
else
   {
   p = (char *) XtRealloc(ptr, size);
   if (p == NULL)
      {
      (void) fprintf
         (stderr, "couldn't XtRealloc %d bytes in %s at %d!!!\n", size, file, line);
      abort();
      /* NOTREACHED */
      } 
   else
      if (p == ptr)
         {
         QPRINTF((stderr,"XtRealloc %d bytes in %s at %d\n", size, file, line));
         }
      else
         {
         QPRINTF((stderr,"%x Freeing (psuedo) storage in %s at %d.\n", ptr, file, line));
         QPRINTF((stderr,"%x XtMalloc %d additional bytes in %s at %d\n", p, size, file, line));
         }
   }

return (p);
      
} /* end of _Realloc */
