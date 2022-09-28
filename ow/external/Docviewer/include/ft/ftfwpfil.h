/*static char *sccsid="@(#) ftfwpfil.h (1.1)  14:21:05 07/08/97";

   ftfwpfil.h -- definitions for WordPerfect filter
	This file is included by:
		ftfwpfil.c
		ftfwpop.c

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------
  | ded 5.1B 91/06/19 | Re-entrancy changes
  | ded 5.1W 91/03/01 | 90120401:Defined poff[2] added mjrver, minver
  | bf  5.0W 90/11/21 | Created
  |-------------------|---------------------------------

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */


	/* set the EDIT define if WordPerfect executable is available */
#ifdef	FTHDOSONLY
#ifndef	FTHWIN
#define	EDIT	"wp"
#endif
#endif
#ifdef	vms
#define EDIT	"wp"
#endif

	/* FTWP structure stored in ftfb_appl */
typedef struct _FTWP {
	long	docstrt;
	USHORT	left;
	USHORT	right;
	USHORT	pitch;
	char	ncsid;
	char	flags;
	char	mjrver; /* Major Version number */
	char	minver; /* Minor Version number */
	UCHAR	FAR *ototab;
	UCHAR	FAR *mtmtab;
	USHORT	(FAR *mtmoff)[2];
	FTNTAB	xtab;
} FTWP;

	/* Values for FTWP.flags */
#define FTWPXSUM	0x01	/* Do not print summary */
#define FTWPW50		0x02	/* WP 5.0 or greater (ie. not plain text) */
#define FTWPBRK		0x04	/* Insert document breaks for online docs */

#define AZLEN	4
#define TZLEN	4
#define CZLEN	5
#define DZLEN	2
#define LINELEN	80
#define IONLN	2
#define IOFFLN	3
#define CHLEN	11
#define HLEN	24
#define DHLEN	22
#define ACHLEN	10
#define KHLEN	12

#define FTC1	0x31	/* Ful/Text '1' */
#define FTC2	0x32	/* Ful/Text '2' */
#define FTC3	0x33	/* Ful/Text '3' */
#define FTC4	0x34	/* Ful/Text '4' */
#define FTC5	0x35	/* Ful/Text '5' */
#define FTC6	0x36	/* Ful/Text '6' */
#define FTC7	0x37	/* Ful/Text '7' */
#define FTC8	0x38	/* Ful/Text '8' */
#define FTSQO	0x5b	/* Ful/Text '[' */
#define FTSQC	0x5d	/* Ful/Text ']' */

/* poff[0] */
	/* counter for number of text characters remaining in current item */
#define TEXTLEN  0x0000ffffL
	/* number of characters left to process in footnote */
#define FOOTLEN  0xffff0000L

/* poff[1] */
	/* following bits define the local "state" of the filbuf function */
#define FLAGBITS 0x001fff00L
	/* currently at start of display line */
#define STARTL	 0x00000100L
	/* in a document summary */
#define SUMMARY  0x00000200L
	/* inside a footnote */
#define NOTE	 0x00000400L
	/* tab rack initialized */
#define TRACK	 0x00000800L
	/* column mode */
#define COLS	 0x00001000L
	/* in a comment */
#define COMMENT  0x00002000L
	/* starting comment/summary */
#define STRTBOX  0x00004000L
	/* ending comment/summary */
#define ENDBOX	 0x00008000L
	/* wp < 5.0 -- got first summary */
#define GOTSUM	 0x00010000L
	/* wp 5.0 -- processing auto reference */
#define AUTOREF  0x00010000L
	/* writing to catalog */
#define CAT	 0x00020000L
	/* processing tabrack */
#define DOTABS	 0x00040000L
	/* processing paragraph numbering or overstrike */
#define DISPLAY	 0x00080000L
	/* Text output has started - do not supress form-feed */
#define TEXTSTRT 0x00100000L
	
	/* Spare bits */
#define SPARE	 0x00e00000L
	/* save left margin */
#define LEFTM	 0xff000000L

/* poff[2] */
	/* Flag bits for flg2 */
#define FLGBITS2 0x00001000L

	/* Field number in Document summary */
#define FLDNO	0x00000f00L
	/* in Description Name of a Document Summary - Wordperfect >=5.1 */
#define DESCNAME 0x00001000L
	/* Length of Description Name of a Document Summary for
	   Wordperfect 5.1 or greater */
#define DESCNMLN 0x001fe000L
/* end of poff[2] */

#define DOTEXT	 (DISPLAY|AUTOREF|COMMENT|NOTE)

extern int FTLOADDS PASCAL ftfwpfil FTPL((FTFB FAR *));
extern int FTLOADDS PASCAL ftfwpffl FTPL((FTFB FAR *));
