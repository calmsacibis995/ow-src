/*static char *sccsid="@(#) ftxtypes.h (1.3)  14:21:09 07/08/97";			

   ftxtypes.h -- public typedefs for index browse functions		

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------		
  | bf  5.0W 91/01/11 | move prototypes to ftgfuncs.h
  | rtk 5.0C 90/06/07 | avoid nested comments				
  | bf	5.0  89/11/06 | Created.					
  |-------------------|---------------------------------		

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */

#ifndef	ftxtyp_h
#define ftxtyp_h
#ifndef	ftgcon_h
#include "ftgconst.h"
#endif

	/* index browse offset structure */
typedef	struct	_FTXOFF	{
	long	xooffset;               /* dictionary offset            */
	long	xoroff;                 /* reference file offset        */
	long	xonvcc;                 /* number of VCC's              */
	long	xoncid;                 /* number of CID's              */
	long	xocurent;               /* offset to start of last entry*/
	int	xorc;                   /* internal ftx return code     */ 
	USHORT 	xostate;                /* internal ftx state           */
	USHORT	xolen;                  /* length of word in xoword     */
	UCHAR	xoword[FTGWORDSZ+2];    /* current word and zone        */
	UCHAR	xopattern[FTGWORDSZ];   /* current pattern              */
}	FTXOFF;

	/* function prototypes */
#include "ftgfuncs.h"

#endif	/*!ftxtyp_h*/
