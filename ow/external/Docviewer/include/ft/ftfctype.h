/*static char *sccsid="@(#) ftfctype.h (1.3)  14:21:04 07/08/97";

   ftfctype.h -- Character class macros to replace ALL uses of the
		 macros normally found in <ctype.h> for the Ful/Text
		 Character Set (FTCS).

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------
  | cps 5.1A 91/02/15 | Add FTFCEOD, FTFCWGHT, FTFCFWGHT
  | cps 5.1A 91/02/11 | Add FTFUK
  | bf  5.0W 90/11/06 | remove ftucstr declaration
  | GA  5.0D 90/09/12 | FTFD4
  | ah  5.0D 90/08/27 | Added definition for 'y' (FTFLY)
  | DAT 5.0D 90/08/11 | SM: VM port (FTFD8)
  | swr 5.0D 90/07/24 | Mac: removed redefinition of FTFNL made initially
  | swr 5.0A 89/12/19 | Macintosh MPW Port
  | KRL 4.6  89/04/24 | FTFD2, FTFSLASH
  | DG	4.6  88/12/23 | change ftfnat() macro to receive 1-1 table pointer
  |-------------------|---------------------------------

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */

#ifndef ftfctyp_h
#define ftfctyp_h 1

#ifndef fthio_h
#include "fthio.h"
#endif
#ifndef ftgcon_h
#include "ftgconst.h"
#endif

extern UCHAR	ftfisal[];
	/* used by ftpglob.h */
#define _FTFCU		0x01	/* Upper Case	*/
#define _FTFCL		0x02	/* Lower Case	*/

/*  Symbolic definitions of invariant values in the Ful/Text character set. */
#define FTFNUL	    0x00    /* NUL - zero byte	*/
#define FTFBS	    0x08    /* backspace	*/
#define FTFTAB	    0x09    /* tab		*/
#define FTFNL	    0x0a    /* newline		*/
#define FTFVT	    0x0b    /* vertical tab	*/
#define FTFFF	    0x0c    /* formfeed 	*/
#define FTFCR	    0x0d    /* return		*/
#define FTFCTRLZ    0x1a    /* control-Z	*/
#define FTFSPACE    0x20    /* Blank		*/
#define FTFEXCL     0x21    /* exclamation '!' - first GL printable char */
#define FTFDQUOT    0x22    /* double quotes	*/
#define FTFPOUND    0x23    /* pound sign	*/
#define FTFDOLR     0x24    /* dollar sign	*/
#define FTFPRCNT    0x25    /* % - percent sign */
#define FTFAMPER    0x26    /* & - ampersand	*/
#define FTFAPOST    0x27    /* apostrophe	*/
#define FTFASTER    0x2a    /* asterisk 	*/
#define FTFIPLUS    0x2b    /* '+' increment VCC			*/
#define FTFCOMMA    0x2c    /* comma		*/
#define FTFHYP	    0x2d    /* hyphen  '-'	*/
#define FTFPERIOD   0x2e    /* period		*/
#define	FTFSLASH    0x2f    /* slash '/'	*/
	/* control sequence parameter delimiters			*/
#define FTFCSCOL    0x3a    /* control sequence parm:value separator	*/
#define FTFCSSEP    0x3b    /* control sequence parm;parm  separator	*/
#define FTFLT	    0x3c    /* less than	*/
#define FTFEQUAL    0x3d    /* equal sign	*/
#define FTFGT	    0x3e    /* greater than	*/
#define FTFQUEST    0x3f    /* question mark	*/
#define FTFBACKSL   0x5c    /* backslash	*/
#define FTFNEL	    0x85    /* next line	*/
#define FTFPLD	    0x8b    /* partial line down	*/
#define FTFPLU	    0x8c    /* partial line up		*/
#define FTFSS2	    0x8e    /* single-shift 2		*/
#define FTFSS3	    0x8f    /* single-shift 3		*/

/* special FTCS codes used for recognizing CSI sequences. */
	/* control sequence introducers:  CSI == ESC[			*/
#define FTFCSI	    0x9b    /* control sequence introducer		*/
#define FTFESC	    0x1b    /* escape					*/
#define FTFESCSI    0x5b    /* left bracket for CSI			*/
	/* control sequence intermediate function codes 		*/
#define FTFIEXCL    FTFEXCL /* reserved for user interface		*/
#define FTFIDIF     FTFDOLR /* '$' DIF (document interchange format)	*/
#define FTFIQUOT    FTFAPOST	/* FTATTR without word separation	*/
#define	FTFIDOLR    FTFDOLR /* '$' DIF (document interchange format) or */
			    /*     embedded term weight sequence	*/
	/* control sequence final function codes			*/
#define FTFCHDSP    0x48    /* 'H' hard space function code		*/
#define FTFCHDHYP   0x49    /* 'I' hard hyphen function code		*/
#define FTFCSFHYP   0x61    /* 'a' soft hyphen function code		*/
#define FTFCSGR     0x6d    /* 'm' SGR (select graphic rendition)	*/
#define FTFCSPRV    0x70    /* 'p' start of private range of fn codes	*/
#define FTFCZSEL    0x73    /* 's' select zone				*/
#define FTFCFTAT    0x76    /* 'v' FTATTR (select Ful/Text attribute)	*/
#define	FTFCWGHT    0x73    /* 's' embedded term weight (fticfseq())	*/
#define	FTFCFWGHT   0x53    /* 'S' embedded term weight (fticfact())	*/
#define	FTFCEOD	    0x4B    /* 'K' embedded end-of-document sequence    */

/* Misc. definitions of possibly variant values in the Ful/Text character
   set. These are used to avoid hex/octal/decimal values in modules that
   operate on specific Ful/Text characters or character ranges.  */
#define FTFCOL2     0x20    /* left nibble of all characters in column 2 */
#define FTFCOL3     0x30    /* left nibble of all characters in column 3 */
#define FTFD0	    0x30    /* Digit '0'	*/
#define FTFD1	    0x31    /* Digit '1'	*/
#define FTFD2	    0x32    /* Digit '2'	*/
#define FTFD4	    0x34    /* Digit '4'	*/
#define FTFD6	    0x36    /* Digit '6'	*/
#define FTFD7	    0x37    /* Digit '7'	*/
#define FTFD8	    0x38    /* Digit '8'	*/
#define FTFD9	    0x39    /* Digit '9'	*/
#define FTFCOL4     0x40    /* left nibble of all characters in column 4 */
#define FTFUA	    0x41    /* Uppercase 'A'	*/
#define FTFUC	    0x43    /* Uppercase 'C' - used in "\C" for FTFCSI   */
#define FTFUE	    0x45    /* Uppercase 'E' 	*/
#define	FTFUK	    0x4b    /* Uppercase 'K'    */
#define FTFUZ	    0x5a    /* Uppercase 'Z'	*/
#define FTFUSC	    0x5f    /* underscore '_' - last code in column 5	 */
#define FTFLA	    0x61    /* Lowercase 'a'	*/
#define FTFLY	    0x79    /* Lowercase 'Y'	*/
#define FTFLZ	    0x7a    /* Lowercase 'z'	*/
#define FTFTILDE    0x7e    /* tilde '~' - last printable code in col 7  */
#define FTFUEXCL    0xa1    /* first GR printable character		 */
#define FTFACBEG    0xc0    /* first non-spacing accent character	 */
#define FTFACEND    0xcf    /* last  non-spacing accent character	 */
#define FTFCOL14    0xe0    /* first GR alphabetic			 */
#define FTFLNIB     0xf0    /* mask to isolate left nibble of a char code*/
#define FTFGRAEND   0xfe    /* last  GR alphabetic			 */


/* Various macros for character class checking and testing. */
	/* Remember <80 cols for MVS */
#define ftfspace(c) ((c)==0x20||(c)==FTFNL||(c)==FTFCR||(c)==FTFTAB||(c)==FTFFF)
#define ftfprint(c) ((c) & 0x60)
#define ftfnprint(c) ((c) >= FTFSPACE && (c) <= 0x7E)
#define ftfcntrl(c) (!((c) & 0x60))
#define ftfdigit(c) ((c) >= FTFD0 && (c) <= FTFD9)
#define ftfalpha(c) (((c)>=FTFUA&&(c)<=FTFUZ)||((c)>=FTFLA&&(c)<=FTFLZ)\
	||(((UCHAR)c)>=FTFCOL14&&((UCHAR)c)<=FTFGRAEND))
#define ftftupper(c) (((c)>=FTFLA&&(c)<=FTFLZ) ? ((c) - FTFLA + FTFUA) : \
	(((UCHAR)c)>=FTFCOL14 && ftfisal[(c)&0x0f]&(_FTFCL)) ? ((c)&0xef) : (c))
#define ftftlower(c) (((c)>=FTFUA&&(c)<=FTFUZ) ? ((c) - FTFUA + FTFLA) : \
	(((UCHAR)c)>=FTFCOL14 && ftfisal[(c)&0x0f]&(_FTFCU)) ? ((c)|0x10) : (c))

/* Translate from FTCS to the Native Character Set (NCS) */
#define ftfnat(c,oto)   ((oto[c]==0||oto[c]==0xff) ? 0x7e : oto[c])
#ifdef FTHASCII
#define ftfsncs(s,b) ((b)?(ftucstr(b,s),((char*)b)):(s))
#else
#ifdef FTHEBCDIC
extern char	*ftfsncs();
#endif
#endif
#endif /* ftfctyp_h */
