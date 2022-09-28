/*static char *sccsid="@(#) ftstypes.h (1.3)  14:21:09 07/08/97";			

   ftstypes.h -- public typedefs for search functions			

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------		
  | bf  5.0W 91/01/11 | move prototypes to ftgfuncs.h
  | hjb	5.0W 90/11/14 | argument pointers FAR for DLL
  | rtk 5.0C 90/06/07 | avoid nested comments				
  | bf  5.0A 89/12/08 | add sflags element to FTSDATA			
  | bf  5.0  89/11/09 | move FTSDATA structure from ftsdata.h		
  | bf	5.0  89/11/06 | Created.					
  |-------------------|---------------------------------		

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */

#ifndef	ftstyp_h
#define ftstyp_h

	/* search result offset structure */
typedef	struct	_FTSOFF	{
	FTVCC	sopos;			/* term's starting position */
	int	sodbndx;		/* relative collection index */
	int	sorelp;			/* relative term position */
	FTCID	socid;			/* catalog id */
	FTCID	sorndx;			/* relative cid index */
}	FTSOFF;

typedef struct	_FTSDATA {
	FTCID	sndocs;			/* # of docs found so far */
	FTCID	svdocs[FTGBBSLIM];	/*     "     per collection */
	FTCID	sccid;			/* current document */
	int	sncoll;			/* numbh passed to ftshopen */
	int	scndx;			/* current collection index */
	int	sflags;
} FTSDATA;

	/* function prototypes */
#include "ftgfuncs.h"

#endif	/*!ftstyp_h*/
