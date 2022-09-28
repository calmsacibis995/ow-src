#pragma ident	"@(#)strutil.c	302.4    97/03/26 lib/libXol SMI"     /* olmisc:strutil.c 1.3 */

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


#include <stdio.h>
#include <string.h>
#include <widec.h>

#include <Xol/memutil.h>
#include <Xol/strutil.h>


/*
 * strmch
 *
 * The \fIstrmch\fR function is used to compare two strings \fIs1\fR
 * and \fIs2\fR.  It returns the index of the first character which 
 * does not match in the two strings.  The value -1 is returned if 
 * the strings match.
 *
 * See also:
 *
 * strnmch(3)
 *
 * Synopsis:
 *
 *#include <strutil.h>
 * ...
 */

extern int strmch(char *s1, char *s2)
{

char * save = s1;

if(s1 == NULL || s2 == NULL)
	return 0;		/* defend against NULL pointers */

while (*s1 == *s2 && *s1 && *s2)
   {
   s1++;
   s2++;
   }
return (*s1 == *s2) ? -1 : s1 - save;

} /* end of strmch */
/*
 * strnmch
 *
 * The \fIstrnmch\fR function is used to compare two strings \fIs1\fR
 * and \fIs2\fR through at most \fIn\fR characters and return the
 * index of the first character which does not match in the two strings.
 * The value -1 is returned if the strings match for the specified number
 * of characters.
 *
 * See also:
 *
 * strmch(3)
 *
 * Synopsis:
 *
 *#include <strutil.h>
 * ...
 */

extern int strnmch(char *s1, char *s2, int n)
{
char * save = s1;

if(s1 == NULL || s2 == NULL)
	return 0;		/* defend against NULL pointers */

while (*s1 == *s2 && n)
   {
   s1++;
   s2++;
   n--;
   }

return (*s1 == *s2) ? -1 : s1 - save;

} /* end of strnmch */

/*
 * strndup
 *
 * The \fIstrndup\fR function is used to create a null-terminated copy of 
 * the first \fIl\fR characters stored in \fIs\fR.
 *
 * Synopsis:
 *
 *#include <strutil.h>
 * ...
 */

extern char * strndup(char *s, int l)
{
char * p = MALLOC(l + 1);

if (p)
   {
   memcpy(p, s, l);
   p[l] = '\0';
   }

return (p);

} /* end of strndup */

wchar_t * wsndup(wchar_t *s, int l)
{
wchar_t * p = (wchar_t *)MALLOC((l + 1)*sizeof(wchar_t));

if (p)
   {
   wsncpy(p, s, l);
   p[l] = L'\0';
   }

return ((wchar_t *)p);

} /* end of wsndup */

