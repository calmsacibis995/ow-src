/*static char *sccsid="@(#) ftntypes.h (1.3)  14:21:08 07/08/97";			

   ftntypes.h -- public typedefs for character set translation functions

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------		
  | bf  5.0W 91/01/11 | move prototypes to ftgfuncs.h
  | hjb	5.0W 90/11/15 | DLL repackaging
  | rtk 5.0C 90/06/07 | avoid nested comments				
  | bf	5.0  89/11/06 | Created.					
  |-------------------|---------------------------------		

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */

#ifndef	ftntyp_h
#define ftntyp_h

typedef struct  _FTNTAB {    /* table used by translation filter */
	char    nid;            	/* NCS id */
	UCHAR   FAR *outtab1;         	/* 1-1 table for output filter */
	UCHAR   FAR *outtab2;    	/* m-n table for output filter */
	USHORT	(FAR *outoff2)[2];    	/* offsets for outtab2 */
	UCHAR   FAR *intab1;          	/* 1-1 table for input filter */
	UCHAR   FAR *intab2;     	/* m-n table for input filter */
	USHORT	(FAR *inoff2)[2];     	/* offsets for intab2 */
} FTNTAB;

	/* function prototypes */
#include "ftgfuncs.h"

#endif	/*!ftntyp_h*/
