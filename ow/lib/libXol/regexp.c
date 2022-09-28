#pragma ident	"@(#)regexp.c	302.6    97/03/26 lib/libXol SMI"     /* olmisc:regexp.c 1.2 */

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
#include <stdlib.h>
#include <widec.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/buffutil.h>
#include <Xol/memutil.h>
#include <Xol/regexp.h>
#include <Xol/strutil.h>


typedef Bufferof(char *) StringBuffer;
typedef Bufferof(wchar_t *) WCStringBuffer;

typedef struct _Match
   {
   StringBuffer * buffer;
   char           matchany;
   char           once;
   } Match;

typedef struct _WCMatch
   {
   WCStringBuffer * buffer;
   char		matchany;
   wchar_t      once;
   } WCMatch;

#define ANYCHAR    ""
#define ANYCHARS   NULL
#define EXPINCRE   10
#define MATCHINCRE 10

static Match * expcmp(char *exp);
static Match * mbexpcmp(char *exp);
static WCMatch * wcexpcmp(wchar_t *exp);

static char * match_begins;
static wchar_t * wcmatch_begins;

/*
 * expcmp
 *
 */

static Match * expcmp(char *exp)
{
static Match * match = NULL;
static char * prev  = NULL;

register int i = 0;
register int s;
char * current;

if ((prev == NULL) || (strcmp(prev, exp) != 0))
   {
   int l = strlen(exp);

   if (prev != NULL)
      {
      FREE(prev);
      for (i = 0; i < match-> buffer-> used; i++)
         if (match-> buffer-> p[i] != ANYCHAR &&
             match-> buffer-> p[i] != ANYCHARS)
            FREE(match-> buffer-> p[i]);
      }
   else
      {
      match = (Match *) MALLOC(sizeof(Match));
      match-> buffer = (StringBuffer *)AllocateBuffer(sizeof(char *), EXPINCRE);
      }

   prev                  = strcpy(MALLOC((unsigned)strlen(exp) + 1), exp);
   match-> buffer-> used = 0;
   match-> once          = (*exp == '^') ? 1 : 0;
   match-> matchany      = 0;

   for (i = match-> once; i < l; i++)
      {
      if (BufferFilled(match-> buffer))
         GrowBuffer((Buffer *)match-> buffer, EXPINCRE);
      switch (exp[i])
         {
         case '[':
            for (s = ++i; i < l && exp[i] != ']';i++)           ;
            current = strndup(&exp[s], i-s);
            break;
         case '*':
            if (BufferEmpty(match-> buffer))
               match-> matchany = 1;
/*
            else
*/
               current = ANYCHARS;
            break;
         case '?':
            current = ANYCHAR;
            break;
         default:
            current = strndup(&exp[i], 1);
            break;
         }
      match-> buffer-> p[match-> buffer-> used++] = current;
      }

   while (match-> buffer-> used != 0 && 
          match-> buffer-> p[match-> buffer-> used - 1] == ANYCHARS)
      match-> buffer-> used--;
   }

return (match);

} /* end of expcmp */


static Match * mbexpcmp(char *exp)
{
static Match * match = NULL;
static char * prev  = NULL;

register int i = 0;
register int s;
char * current;


if ((prev == NULL) || (strcmp(prev, exp) != 0))
   {
   int l = strlen(exp);

   if (prev != NULL)
      {
      FREE(prev);
      for (i = 0; i < match-> buffer-> used; i++)
         if (match-> buffer-> p[i] != ANYCHAR &&
             match-> buffer-> p[i] != ANYCHARS)
            FREE(match-> buffer-> p[i]);
      }
   else
      {
      match = (Match *) MALLOC(sizeof(Match));
      match-> buffer = (StringBuffer *)AllocateBuffer(sizeof(char *), EXPINCRE);
      }

   prev              = strcpy(MALLOC((unsigned)strlen(exp) + 1), exp);
   match-> buffer-> used = 0;
   match-> once          = (*exp == '^') ? 1 : 0;
   match-> matchany      = 0;

   for (i = match-> once; i < l; i += mblen(&exp[i],MB_CUR_MAX))
      {
      if (BufferFilled(match-> buffer))
         GrowBuffer((Buffer *)match-> buffer, EXPINCRE);
      switch (exp[i])
         {
         case '[':
            for (s = i = i + mblen(&exp[i],MB_CUR_MAX); i < l && exp[i] != ']';
					i += mblen(&exp[i],MB_CUR_MAX))
			           ;
            current = strndup(&exp[s], i-s);
            break;
         case '*':
            if (BufferEmpty(match-> buffer))
               match-> matchany = 1;
/*
            else
*/
               current = ANYCHARS;
            break;
         case '?':
            current = ANYCHAR;
            break;
         default:
            current = strndup(&exp[i], mblen(&exp[i],MB_CUR_MAX));
            break;
         }
      match-> buffer-> p[match-> buffer-> used++] = current;
      }

   while (match-> buffer-> used != 0 && 
          match-> buffer-> p[match-> buffer-> used - 1] == ANYCHARS)
      match-> buffer-> used--;
   }

return (match);

} /* end of mbexpcmp */


/*
 * wcexpcmp
 *
 */
extern wchar_t *wsndup(wchar_t *s, int l);
extern WCStringBuffer *WCAllocateBuffer (int element_size, int initial_size);
extern void WCGrowBuffer (WCStringBuffer *b, int increment);

static WCMatch * wcexpcmp(wchar_t *exp)
{
static WCMatch * match = NULL;
static wchar_t * prev  = NULL;

register int i = 0;
register int s;
wchar_t *current;
char	mbchar[4];

if ((prev == NULL) || (wscmp(prev, exp) != 0))
   {
   int l = wslen(exp);

   if (prev != NULL)
      {
      FREE((char *)prev);
      for (i = 0; i < match-> buffer-> used; i++ )
         if (match-> buffer-> p[i] != (wchar_t *)"" &&
             match-> buffer-> p[i] != (wchar_t *)NULL)
            FREE((char *)match-> buffer-> p[i]);
      }
   else
      {
      match = (WCMatch *) MALLOC(sizeof(WCMatch));
      match-> buffer = (WCStringBuffer *)WCAllocateBuffer(sizeof(wchar_t *), EXPINCRE);
      }

   prev = wscpy((wchar_t *)MALLOC( ((unsigned)wslen(exp) + 1)*sizeof(wchar_t)),
									 exp);
   match-> buffer-> used = 0;
   match-> once          = (*exp == L'^') ? 1 : 0;
   match-> matchany      = 0;

   for (i = match-> once; i < l; i++)
      {
      if (BufferFilled(match-> buffer))
         WCGrowBuffer((WCStringBuffer *)match-> buffer, EXPINCRE);
	wctomb(mbchar , exp[i]);
      switch (*mbchar)
         {
         case '[':
            for (s = ++i; i < l && exp[i] != L']'; i++)
				;
            current = wsndup(&exp[s], i-s);
            break;
         case '*':
            if (BufferEmpty(match-> buffer))
               match-> matchany = 1;
/*
            else
*/
               current = (wchar_t *)NULL;
            break;
         case '?':
            current = (wchar_t *)"";
            break;
         default:
            current = wsndup(&exp[i], 1);
            break;
         }

      match-> buffer-> p[match-> buffer-> used++] = current;

      }

   while (match-> buffer-> used != 0 && 
      match-> buffer-> p[match-> buffer-> used - 1] == (wchar_t *)NULL)
      match-> buffer-> used--;
   }
      match-> buffer-> p[match-> buffer-> used] = (wchar_t *)"";

return (match);

} /* end of wcexpcmp */

/*
 * strexp
 *
 * The \fIstrexp\fR function is used to perform a regular expression forward
 * scan of \fIstring\fR for \fIexpression\fR starting at \fIcurp\fR.
 * The regular expression language used is:
 *
 * .so CWstart
 *        c - match c
 *  [<set>] - match any character in <set>
 * [!<set>] - match any character not in <set>
 *        * - match any character(s)
 *        ^ - match must start at curp
 * .so CWend
 *
 * Return value:
 *
 * NULL is returned if expression cannot be found in string; otherwise
 * a pointer to the first character in the substring which matches
 * expression is returned.  The fucntion streexp(3) can be used to get
 * the pointer to the last character in the match.
 *
 * See also:
 *
 * strrexp(3), streexp(3)
 *
 * Synopsis:
 *
 *#include <expcmp.h>
 * ...
 */

extern char * strexp(char *string, char *curp, char *expression)
{
Match * match = NULL;
char * found = NULL;

if ( curp == NULL  ||  expression == NULL ||
    *curp == '\0'  || *expression == '\0')
   return(NULL);

if ((match = expcmp(expression)) != NULL)
   {
   register int matchpos;
   char * pos;
   char * matchstart;
   char * stringend = &curp[strlen(curp)];

   if (match-> once && string != curp)
      return (NULL);

   matchstart = pos = curp;
   matchpos = 0;

   if (match-> buffer-> used != 0)
   do
      {

      if (matchpos < match-> buffer-> used &&
	match-> buffer-> p[matchpos] != ANYCHARS &&
  (*match-> buffer-> p[matchpos] == '\0' ||
  (*match-> buffer-> p[matchpos] != '!' && 
			strchr(match-> buffer-> p[matchpos], *pos) != NULL) ||
  (*match-> buffer-> p[matchpos] == '!' && 
			strchr(match-> buffer-> p[matchpos], *pos) == NULL)))
         {
         match-> matchany = 0;
         if (matchpos == match-> buffer-> used - 1)
            match_begins = pos;
         if (++matchpos == match-> buffer-> used)
            found = matchstart;
         }
      else
         if (match-> buffer-> p[matchpos] == ANYCHARS)
            {
            for (; matchpos < match-> buffer-> used; matchpos++)
               if (match-> buffer-> p[matchpos] != ANYCHARS)
                  break;
            match-> matchany = 1;
            }
         else
            if (!match-> matchany)
               {
	       /* If a partial match failed, we need to start match from
		* that location.
		*/
	       if (matchpos)
		  {
		  matchstart = pos;
		  pos--; /* it will be incremented to current value outside */
		  }
	       else
		  {
                  matchstart = pos + 1;
		  }
               matchpos = 0;
               if (match-> once) break;
               }
      pos++;
      } while (found == NULL && pos != stringend);
   }

return (found);

} /* end of strexp */

extern char * mbstrexp(char *string, char *curp, char *expression)
{
Match * match = NULL;
char * found = NULL;

if ( (curp == NULL  ||  expression == NULL) ||
    (*curp == '\0'  || *expression == '\0'))
   return(NULL);

if ((match = mbexpcmp(expression)) != NULL)
   {
   register int matchpos;
   char * pos;
   char * matchstart;
   char * stringend = &curp[strlen(curp)];

   if (match-> once && string != curp)
      return (NULL);

   matchstart = pos = curp;
   matchpos = 0;

   if (match-> buffer-> used != 0)
   do
      {
	int	i;
	char	mbchar[8];

	for(i = 0; i < mblen(pos,MB_CUR_MAX);i++)
		mbchar[i] = pos[i];
	mbchar[i] = '\0';

      if (matchpos < match-> buffer-> used &&
	match-> buffer-> p[matchpos] != ANYCHARS &&
  (*match-> buffer-> p[matchpos] == '\0' ||
  (*match-> buffer-> p[matchpos] != '!' && 
	strstr(match-> buffer-> p[matchpos], mbchar) != NULL) ||
  (*match-> buffer-> p[matchpos] == '!' && 
	strstr(match-> buffer-> p[matchpos], mbchar) == NULL)))
         {
         match-> matchany = 0;
         if (matchpos == match-> buffer-> used - 1)
            match_begins = pos;
         if (++matchpos == match-> buffer-> used)
            found = matchstart;
         }
      else
         if (match-> buffer-> p[matchpos] == ANYCHARS)
            {
            for (; matchpos < match-> buffer-> used; matchpos++)
               if (match-> buffer-> p[matchpos] != ANYCHARS)
                  break;
            match-> matchany = 1;
            }
         else
            if (!match-> matchany)
               {
               matchpos = 0;
               matchstart = pos + mblen(curp,MB_CUR_MAX);
               if (match-> once) break;
               }
      pos = pos + mblen(curp,MB_CUR_MAX);
      } while (found == NULL && pos != stringend);
   }

return (found);

} /* end of mbstrexp */

extern wchar_t * wcstrexp(wchar_t *string, wchar_t *curp, wchar_t *expression)
{
WCMatch * match = (WCMatch *)NULL;
wchar_t * found = (wchar_t *)NULL;

if ( curp == (wchar_t *)NULL  ||  expression == (wchar_t *)NULL ||
    *curp == L'\0'  || *expression == L'\0')
   return((wchar_t *)NULL);

if ((match = wcexpcmp(expression)) != NULL)
   {
   register int matchpos;
   wchar_t * pos;
   wchar_t * matchstart;
   wchar_t * stringend = &curp[wslen(curp)];

   if (match-> once && string != curp)
      return ((wchar_t *)NULL);

   matchstart = pos = curp;
   matchpos = 0;

   if (match-> buffer-> used != 0)
   do
      {
      if (matchpos < match-> buffer-> used &&
	match-> buffer-> p[matchpos] != (wchar_t *)NULL &&
	  (*match-> buffer-> p[matchpos] == L'\0' ||
	  (*match-> buffer-> p[matchpos] != L'!' && 
			wschr(match-> buffer-> p[matchpos],*pos)  != NULL) ||
	  (*match-> buffer-> p[matchpos] == L'!' && 
			wschr(match-> buffer-> p[matchpos],*pos)  == NULL)))
         {
         match-> matchany = 0;
         if (matchpos == match-> buffer-> used - 1)
            wcmatch_begins = pos;
         if (++matchpos == match-> buffer-> used)
            found = matchstart;
         }
      else
         if (match-> buffer-> p[matchpos] == (wchar_t *)NULL)
            {
            for (; matchpos < match-> buffer-> used; matchpos++)
               if (match-> buffer-> p[matchpos] != (wchar_t *)NULL)
                  break;
            match-> matchany = 1;
            }
         else
            if (!match-> matchany)
               {
               matchpos = 0;
               matchstart = pos + 1;
               if (match-> once) break;
               }
      pos++;
      } while (found == (wchar_t *)NULL && pos != stringend);
   }

return ((wchar_t *)found);

} /* end of wcstrexp */

/*
 * strrexp
 *
 * The \fIstrrexp\fR function is used to perform a regular expression backward
 * scan of \fIstring\fR for \fIexpression\fR starting at \fIcurp\fR.
 * The regular expression language used is:
 *
 * .so CWstart
 *        c - match c
 *  [<set>] - match any character in <set>
 * [!<set>] - match any character not in <set>
 *        * - match any character(s)
 *        ^ - match must start at curp
 * .so CWend
 *
 * Return value:
 *
 * NULL is returned if expression cannot be found in string; otherwise
 * a pointer to the first character in the substring which matches
 * expression is returned.  The fucntion streexp(3) can be used to get
 * the pointer to the last character in the match.
 *
 * See also:
 *
 * strexp(3), streexp(3)
 *
 * Synopsis:
 *
 *#include <expcmp.h>
 * ...
 */

extern char * strrexp(char *string, char *curp, char *expression)
{
Match * match = NULL;
char * found = NULL;

if (expression == NULL || *expression == '\0')
   return(NULL);

if (curp == NULL) 
   curp = &string[strlen(string) - 1];

if (curp < string)
   return(NULL);

if ((match = expcmp(expression)) != NULL)
   {
   register int matchpos;
   char * pos;
   char * stringend = string;

   if (match-> once)
      {
      if (string != curp)
	 return (strexp(string, string, expression));
      }
   else
      if ((strexp(curp, curp, expression)) == curp)
         return(curp);

   pos = curp;
   matchpos = match-> buffer-> used - 1;

   if (match-> buffer-> used != 0)
   do
      {
      if (matchpos < match-> buffer-> used &&
  (*match-> buffer-> p[matchpos] == '\0' ||
  (*match-> buffer-> p[matchpos] != '!' && 
	strchr(match-> buffer-> p[matchpos], *pos) != NULL) ||
  (*match-> buffer-> p[matchpos] == '!' && 
	strchr(match-> buffer-> p[matchpos], *pos) == NULL)))
         {
         match-> matchany = 0;
         if (matchpos == match-> buffer-> used - 1)
            match_begins = pos;
         if (--matchpos == -1)
            found = pos;
         }
      else
         if (match-> buffer-> p[matchpos] == ANYCHARS)
            {
            for (; matchpos >= 0; matchpos--)
               if (match-> buffer-> p[matchpos] != ANYCHARS)
                  break;
            match-> matchany = 1;
            }
         else
            if (!match-> matchany)
               {
               matchpos = match-> buffer-> used - 1;
               if (match-> once) break;
               }
      pos--;
      } while (found == NULL && pos >= stringend);
   }

return (found);

} /* end of strrexp */



extern char * mbstrrexp(char *string, char *curp, char *expression)
{
Match * match = NULL;
char * found = NULL;
int	numchar;
int	charcnt;
int	j;

if (expression == NULL || *expression == '\0')
   return(NULL);

for(j = 0 , numchar = 0; j < (int)strlen(string) ;
				j += mblen(&string[j] ,MB_CUR_MAX))
		numchar ++;

if (curp == NULL) 
   curp = &string[j];

if (curp < string)
   return(NULL);

if ((match = mbexpcmp(expression)) != NULL)
   {
   register int matchpos;
   char * pos;
   char * stringend = string;

   if (match-> once)
      {
      if (string != curp)
	 return (mbstrexp(string, string, expression));
      }
   else
      if ((mbstrexp(curp, curp, expression)) == curp)
         return(curp);

   pos = curp;
   matchpos = match-> buffer-> used - 1;

   if (match-> buffer-> used != 0)
   do
      {
        int     i;
        char    mbchar[8];

        for(i = 0; i < mblen(pos,MB_CUR_MAX);i++)
                mbchar[i] = pos[i];
        mbchar[i] = '\0';

      if (matchpos < match-> buffer-> used &&
  (*match-> buffer-> p[matchpos] == '\0' ||
  (*match-> buffer-> p[matchpos] != '!' && 
	strstr(match-> buffer-> p[matchpos], mbchar) != NULL) ||
  (*match-> buffer-> p[matchpos] == '!' && 
	strstr(match-> buffer-> p[matchpos], mbchar) == NULL)))
         {
         match-> matchany = 0;
         if (matchpos == match-> buffer-> used - 1)
            match_begins = pos;
         if (--matchpos == -1)
            found = pos;
         }
      else
         if (match-> buffer-> p[matchpos] == ANYCHARS)
            {
            for (; matchpos >= 0; matchpos--)
               if (match-> buffer-> p[matchpos] != ANYCHARS)
                  break;
            match-> matchany = 1;
            }
         else
            if (!match-> matchany)
               {
               matchpos = match-> buffer-> used - 1;
               if (match-> once) break;
               }

	for(j = 0 , charcnt = 0 , numchar--;
	(j < (int)strlen(string) && charcnt < numchar && numchar >= 0);
				j += mblen(&string[j] ,MB_CUR_MAX)){
		charcnt ++;
	}

	if(numchar > 0)
	      pos = &string[j];
	else
	      pos--; 
      } while (found == NULL && pos >= stringend);
   }

return (found);

} /* end of mbstrrexp */

extern wchar_t * wcstrrexp(wchar_t *string, wchar_t *curp, wchar_t *expression)
{
WCMatch * match = (WCMatch *)NULL;
wchar_t * found = (wchar_t *)NULL;

if (expression == (wchar_t *)NULL || *expression == L'\0')
   return(NULL);

if (curp == (wchar_t *)NULL) 
   curp = &string[wslen(string) - 1];

if (curp < string)
   return((wchar_t *)NULL);

if ((match = wcexpcmp(expression)) != NULL)
   {
   register int matchpos;
   wchar_t * pos;
   wchar_t * stringend = string;

   if (match-> once)
      {
      if (string != curp)
	 return (wcstrexp(string, string, expression));
      }
   else
      if ((wcstrexp(curp, curp, expression)) == curp)
         return(curp);

   pos = curp;
   matchpos = match-> buffer-> used - 1;

   if (match-> buffer-> used != 0)
   do
      {
      if (matchpos < match-> buffer-> used &&
  (*match-> buffer-> p[matchpos] == L'\0' ||
  (*match-> buffer-> p[matchpos] != L'!' && 
	wschr(match-> buffer-> p[matchpos], *pos) != (wchar_t *)NULL) ||
  (*match-> buffer-> p[matchpos] == L'!' && 
	wschr(match-> buffer-> p[matchpos], *pos) == (wchar_t *)NULL)))
         {
         match-> matchany = 0;
         if (matchpos == match-> buffer-> used - 1)
            wcmatch_begins = pos;
         if (--matchpos == -1)
            found = pos;
         }
      else
         if (match-> buffer-> p[matchpos] == (wchar_t *)NULL)
            {
            for (; matchpos >= 0; matchpos--)
               if (match-> buffer-> p[matchpos] != (wchar_t *)NULL)
                  break;
            match-> matchany = 1;
            }
         else
            if (!match-> matchany)
               {
               matchpos = match-> buffer-> used - 1;
               if (match-> once) break;
               }
      pos--;
      } while (found == NULL && pos >= stringend);
   }

return (found);

} /* end of wcstrrexp */

/*
 * streexp
 *
 * The \fIstreexp\fR function is used to retrieve the pointer of the last
 * character in a match found following a strexp/strrexp function call.
 *
 * See also:
 *
 * strexp(3), strrexp(3)
 *
 * Synopsis:
 *
 *#include <regexp.h>
 * ...
 */

extern char * streexp(void)
{

return (match_begins);

} /* end of streexp */

extern WCStringBuffer *
WCAllocateBuffer (int element_size, int initial_size)
{
WCStringBuffer * b = (WCStringBuffer *) MALLOC(sizeof(WCStringBuffer));

b-> esize = element_size;
b-> size = b-> used = 0;
b-> p = (wchar_t **)NULL;

if (initial_size != 0)
   WCGrowBuffer(b, initial_size);

return (b);

} /* end of WCAllocateBuffer */

extern void
WCGrowBuffer (WCStringBuffer *b, int increment)
{
b-> size += increment;

if (b-> size == increment)
   b-> p = (wchar_t **)MALLOC((unsigned)(b-> size * b-> esize));
else
   b-> p = (wchar_t **)REALLOC((char *)b-> p, (unsigned)(b-> size * b-> esize));

} /* end of WCGrowBuffer */

