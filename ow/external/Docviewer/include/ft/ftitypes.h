/*static char *sccsid="@(#) ftitypes.h (1.3)  14:21:07 07/08/97";

   ftitypes.h -- public typedefs for indexing functions

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------
  | bf  5.0W 91/01/11 | move function prototypes to ftgfuncs.h
  | hjb	5.0W 90/11/12 | DLL repackaging
  | KRL 5.0D 90/06/21 | istates from ftgconst.h
  | bf  5.0C 90/05/24 | change size of ifn from FTGPATHSZ to FTNPATHSZ
  | bf	5.0  89/11/27 | new istates: FTIQUICK,FTSQUICK; drop FTBNOWAIT
  | bf	5.0  89/11/16 | iflags
  |-------------------|---------------------------------

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */


#ifndef	ftityp_h
#define	ftityp_h

	/* state values for istate element of FTIDATA structure */
#define FTISTART	0	/* startup */
#define FTISSCAN	1	/* start catalog scan */
#define FTIFSCAN	2	/* finish catalog scan */
#define FTIPARSE	3	/* parse a document */
#define FTISSORT	4	/* sort a segment */
#define FTIPIMRG	5	/* periodic interim merge */
#define FTIPFMRG	6	/* periodic final merge */
#define FTIIFMRG	7	/* immediate final merge */
#define FTICATUP	8	/* update catalog status flags */
#define FTIIDONE	9	/* indexing complete */
#define FTIIFAIL	10	/* indexing failed (log file updated) */
#define FTIICANC	11	/* indexing cancelled */
#define FTISWALK	12	/* start tree walk of a directory */
#define FTIFWALK	13	/* finish tree walk */
#define FTIIFAIL2	14	/* indexing failed (log file not updated) */
	/* undocumented states */
#define FTILINIT	15	/* open collection */
#define FTILCCHK	16	/* begin catalog check */
#define FTILFCHK	17	/* begin file system check */
#define FTILWPAR	18	/* begin document parsing */
#define FTILIXUP	19	/* begin final index merge */
#define FTILYMRG	20	/* begin immediate index merge */
#define	FTILERR		21	/* close and exit with error */
#define FTILPERM	22	/* unlock, close, and exit with error */
#define FTILTERM	23	/* unlock, close, and exit normally */

	/* flags for iflags element of FTIDATA structure */
#define FTIQUICK	0x0001

typedef struct _ftidata {
	FTCID	icid;
	int	istate;
	int	iflags;
	char	ifn[FTNPATHSZ];
} FTIDATA;

	/* function prototypes */
#include "ftgfuncs.h"

#endif	/*!ftityp_h*/
