/*static char *sccsid="@(#) ftnctype.h (1.3)  14:21:07 07/08/97";

   ftnctype.h -- Character class macros to replace ALL uses of the
		 macros normally found in <ctype.h> for the Native
		 Character Set (NCS) of the machine.

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------
  | bf  5.0W 90/11/06 | remove ftucstr declaration
  | rl	5.0  89/10/26 | add ftnfctx macro
  | DG	4.6  88/12/23 | change ftnftcs() macro to receive 1-1 table pointer
  |-------------------|---------------------------------

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */

#ifndef ftnctyp_h
#define ftnctyp_h 1

#ifndef ftgcon_h
#include "ftgconst.h"
#endif

/*
 * Bit assignments for each character class. Note that as the table
 *  is defined as a UCHAR array, only 8 choices are used. If more are
 *  required, the table must be enlarged (i.e. USHORT, ULONG)
 *  and ALL code using the macros must be re-compiled.
 */
#define _FTCU		0x01	/* Upper Case	 ('A'-'Z')	    */
#define _FTCL		0x02	/* Lower Case	 ('a'-'z')	    */
#define _FTCN		0x04	/* Numeric Digit ('0'-'9')	    */
#define _FTCS		0x08	/* White Space	 (' ',\t\r\f\n)     */
#define _FTCC		0x10	/* Control Character		    */
#define _FTCX		0x20	/* Hexadecimal Digit (A-F,a-f)	    */
#define _FTCP		0x40	/* Punctuation (Other printables)   */
#define _FTCBL		0x80	/* Blank			    */

/*
 * Special characters in the Native Character Set
 */
#ifdef FTHASCII
#define FTNESC		0x1B	/* Escape	*/
#define FTNCSI		0x9B	/* CSI		*/
#else
#ifdef FTHEBCDIC
#define FTNESC		0x1B	/* Escape	*/
#define FTNCSI		0x9B	/* CSI		*/
#endif
#endif

/* Character class table from 'ftglobal.c' */
extern	UCHAR	ftnccls[];

/* Various macros for character class checking.
 *  Note the use of normalization to ensure that unsigned
 *  array indexing is always performed.
 * '1' is added to '&ftcclass' to account for a reasonable
 *  return from the macros when (EOF) is given as 'c'. All the
 *  entries in 'ftcclass' are therefore offset by '1' to account for (EOF)
 *  handling.
 */
#define ftnalpha(c)  ((ftnccls + 1)[c] & (_FTCL|_FTCU))
#define ftnupper(c)  ((ftnccls + 1)[c] & (_FTCU))
#define ftnlower(c)  ((ftnccls + 1)[c] & (_FTCL))
#define ftndigit(c)  ((ftnccls + 1)[c] & (_FTCN))
#define ftnxdigit(c) ((ftnccls + 1)[c] & (_FTCN|_FTCX))
#define ftnspace(c)  ((ftnccls + 1)[c] & (_FTCS))
#define ftnascii(c)  ((c) < 128)
#define ftnpunct(c)  ((ftnccls + 1)[c] & (_FTCP))
#define ftnalnum(c)  ((ftnccls + 1)[c] & (_FTCL|_FTCU|_FTCN))
#define ftnprint(c)  ((ftnccls+1)[c]&(_FTCL|_FTCU|_FTCN|_FTCP|_FTCBL))
#define ftncntrl(c)  ((ftnccls + 1)[c] & (_FTCC))
#define ftntupper(c) (ftnlower(c) ? ((c) - 'a' + 'A') : (c))
#define ftntlower(c) (ftnupper(c) ? ((c) - 'A' + 'a') : (c))
#define ftmin(a,b)	((a) < (b) ? (a) : (b))
#define ftmax(a,b)	((a) > (b) ? (a) : (b))
#define ftabs(a)	((a) < 0 ? -(a) : (a))

	/* translate 1 NCS char to 1 FTCS */
#define ftnftcs(c,oto)   ((oto[c]==0||oto[c]==0xff) ? 0x7e : oto[c])
#define ftnfctx(c,oto)   (oto[c]==0xff) ? 0x7e : oto[c])
#ifdef FTHASCII
#define ftnsftcs(s,b) ((b)?(ftucstr(b,s),((char*)b)):(s))
#else
#ifdef FTHEBCDIC
extern char *ftnsftcs();
#endif
#endif

#endif
