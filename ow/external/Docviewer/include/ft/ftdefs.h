/*static char *sccsid="@(#) ftdefs.h (1.3)  14:21:03 07/08/97";			

   ftdefs.h -- header file for callers of programmer interface functions

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------		
  | rtk 5.0C 90/06/07 | avoid nested comments				
  | bf  5.0  89/11/17 | drop inclusion of ftotypes			
  | bf  5.0  89/11/09 | include various instead of ftgtypes		
  |-------------------|---------------------------------		

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */

/*	NOTE: In the include files named below, the comment PRIVATE preceding
	a symbol means that its use is defined here for the convenience of
	Fulcrum technical staff only, and should not be used by callers of
	the Ful/Text Application Programmer Interface. */

#ifndef	ftdefs_h
#define	ftdefs_h

#ifndef	fthio_h
#include "fthio.h"
#endif
	
	/* general constants */
#ifndef	ftgcon_h
#include "ftgconst.h"
#endif

	/* typedefs */
#include "fthandle.h"
#include "ftbtypes.h"	/* FTFIG */
#include "ftctypes.h"	/* FTC... */
#include "ftntypes.h"	/* FTNTAB */
#include "ftstypes.h"	/* FTSOFF FTSDATA */
#include "ftutypes.h"	/* FTDEV FTDATR FTFATR FTBCODE */
#include "ftxtypes.h"	/* FTXOFF */

	/* fterrno and symbolic constants for error codes */
#include "fterror.h"

#endif	/*!ftdefs_h*/
