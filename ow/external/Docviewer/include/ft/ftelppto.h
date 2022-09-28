/*static char *sccsid="@(#) ftelppto.h (1.1)  14:21:03 07/08/97";

   ftelppto.h -- prototypes for ELP query filter routines

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------
  | WR  5.1A 91/05/17 | Use ftlpbdrv.h instead of ftlpb.h
  | WR	5.1  91/03/21 | Created.
  |-------------------|---------------------------------

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */

#ifndef	ftelppto_h
#define	ftelppto_h
#include "fthio.h"
#include "ftlpbdrv.h"

	/* FUNCTION PROTOTYPES */
/* WHERE POSSIBLE, PLEASE MAINTAIN IN ALPHABETICAL ORDER BY FUNCTION NAME */
	/* FTPL means prototype used only if FTLINT is defined */
	/* the definition for FTPL is in fthio.h */

extern int ftelpcnv FTPL((FTAPIH, char *, int *, int *));

extern int ftelpop FTPL((FTAPIH, FTELPQST FAR *, int *, int *));

extern int ftelppar FTPL((FTAPIH, FTQIB FAR *));

extern int ftelppro FTPL((FTAPIH, FTQIB FAR *));

extern int ftelpqf FTPL((FTAPIH, FTQIB FAR *));

extern int ftelpsta FTPL((FTAPIH, FTQIB FAR *));

extern int ftelpus FTPL((FTAPIH, FTELPQST FAR *, int, int));

#endif	/*!ftelpto_h*/
