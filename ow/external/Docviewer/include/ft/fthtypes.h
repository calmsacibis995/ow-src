/*static char *sccsid="@(#) fthtypes.h (1.3)  14:21:06 07/08/97";

   fthtypes.h -- header file for callers of Ful/Text API functions 

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------
  | bf  5.0  89/11/20 | Created.
  |-------------------|---------------------------------

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */

/*	NOTE: In the include files named below, the comment PRIVATE preceding
 *	a symbol means that its use is defined here for the convenience of
 *	Fulcrum technical staff only, and should not be used by callers of
 *	the Ful/Text API.
 */

#ifndef	fthtyp_h
#define	fthtyp_h

	/* system-dependent defines */
#ifndef	fthio_h
#include "fthio.h"
#endif
	
	/* general constants */
#ifndef	ftgcon_h
#include "ftgconst.h"
#endif

	/* typedefs */
#ifndef	fthndl_h
#include "fthandle.h"
#endif

	/* fterrno and symbolic constants for error codes */
#ifndef	fterr_h
#include "fterror.h"
#endif

#endif	/*!fthtyp_h*/
