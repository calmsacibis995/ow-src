/*static char *sccsid="@(#) ftfmt.h (1.3)  14:21:04 07/08/97";

   ftfmt.h -- type definitions for word-wrap pseudo-filter

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------
  | ded 5.1C 92/01/08 | 91111401:removed ftfmtcol, it its now a function call
  | swr 5.1B 91/05/27 | 91012901: added #ifdef FTCPP (C++ safe links)
  | hjb	5.0W 91/01/25 | DLL repackaging of ftfminit() as ftfvin(), etc.
  | hjb	5.0W 90/12/14 | prototype and FTVSTATE.stb changes (FAR pointers)
  | swr 5.0D 90/10/19 | 90101804: added FTVFMT to choose ftfmtcol or ftvfmtcol
  | bf  5.0  89/11/07 | include fthandle.h instead of ftgtypes.h
  | bf  4.6  89/04/04 | FTPL
  |-------------------|---------------------------------

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */

#ifndef	FTFMTCOL

#ifdef	FTHSDM
#ifndef	FTFUFILT
	/* old user interface */
#define FTMAXFMT	8		/* max number of unique format states */
#else
	/* u-filter */
#define FTMAXFMT	20
#endif
#define FTFMTCOL	132		/* columns in tab rack */
#else	/*FTHLDM*/
#define FTMAXFMT	50
#define FTFMTCOL	256
#endif	/*FTHLDM*/

#define FTFMTSIZ	FTMAXFMT + 2	/* size of format vector */
#define FTWRKFMT	FTMAXFMT	/* vector element for working format */
#define FTTMPFMT	FTMAXFMT + 1	/* element for temporary changes */
#define FTDFLFMT	0		/* element for defaults */

#ifdef	FTFMTGLB
#ifdef	FTVFMT		/* indicates api/ftvfmt (used by ft, ftpr) */
int	ftvfmtcol = FTFMTCOL;
#endif
#else	/*!FTFMTGLB*/
#ifdef	FTVFMT		/* indicates api/ftvfmt (used by ft, ftpr) */
extern	int	ftvfmtcol;
#endif
#endif	/*!FTFMTGLB*/

/*	NOTE: The following macros are not defined for column values greater
	than FTFMTCOL, even though the right margin may exceed this value.
*/

#ifdef	CLAN7			/* CLAN7 chokes on the tabtop macro */
#define ftfmtchk(rack, col)	fttbstp(rack, col)
#else
#define ftfmtchk(rack, col)	((rack[(col-1) / 8] >> ((col-1) % 8)) & 0x01)
#endif
#define ftfmtset(rack, col)	(rack[(col-1) / 8] |= (0x01 << ((col-1) % 8)))

	/* the FTVSTATE structure is used to remember document format
	   at frame boundaries */
typedef struct _FTVSTATE	{
	USHORT		lmargin,	/* column position of left margin */
			rmargin,	/* column of right margin */
			tindent,	/* column of temporary indent */
			scol,		/* centering; current col of text */
			cntr;		/* centering; col to center on */
	char	FAR	*stb;		/* centering; ptr to text to center */
	UCHAR		tabs[FTFMTCOL / 8 + 1];
} FTVSTATE;

/* The following defines map the zeroeth long from an FTFB into state info */

/* to obtain the values: */
#define ftvsvat(l) \
	(int)(l & 0xffff)		/* video attribute */
#define ftvsndx(l) \
	(int)(((l & 0x00ff0000)>>16)&0x00ff)   /* index into format vector */
#define ftvscol(l) \
	(int)(((l & 0xff000000)>>24)&0x00ff)   /* used only by ftvflout */

/* to set the values: */
#define ftvpvat(l, vat) (l = (l & 0xffff0000) | (vat & 0xffff))
#define ftvpndx(l, ndx) (l = (l & 0xff00ffff) | ((long)(ndx & 0xff) << 16))
#define ftvpcol(l, col) (l = (l & 0x00ffffff) | ((long)(col & 0xff) << 24))

#ifdef	FTLINT
	/* pull in some definitions required for function prototypes */
#ifndef ftfb_h
#include "ftfb.h"	/* FTFB */
#endif
#ifndef fthndl_h
#include "fthandle.h"	/* FTVCC */
#endif
#endif	/*FTLINT*/

#ifdef	FTCPP
extern	"C" {
#endif
extern int	ftfucseq FTPL((FTFB FAR *, int FAR *, int FAR *, int, int,
		    int FAR *, FTVSTATE FAR *, int FAR *, char FAR *,
		    FTVCC FAR *, int, char FAR *, int));
#define	ftfminit(v,r)		ftfvin(ftapih,v,r)
extern int FTAPIFUN ftfvin FTPL((FTAPIH, FTVSTATE FAR *, int));
#define	ftfmcopy(v,t,f)		ftfvcp(ftapih,v,t,f)
extern int FTAPIFUN ftfvcp FTPL((FTAPIH, FTVSTATE FAR *, int, int));
#define	ftfmcmpr(v,a,b)		ftfvcm(ftapih,v,a,b)
extern int FTAPIFUN ftfvcm FTPL((FTAPIH, FTVSTATE FAR *, int, int));
#define	ftfmgetx(v,p)		ftfvix(ftapih,v,p)
extern int FTAPIFUN ftfvix FTPL((FTAPIH, FTVSTATE FAR *, int FAR *));
#define	ftfucntr(v,b,p,c,l)	ftfvct(ftapih,v,b,p,c,l)
extern int FTAPIFUN ftfvct FTPL((FTAPIH, FTVSTATE FAR *, char FAR *,
				    char FAR *, int FAR *, int));
extern int FTAPIFUN ftfmtcol FTPL((FTAPIH, int));

#ifdef	FTCPP
}
#endif

#endif	/*!FTFMTCOL*/
