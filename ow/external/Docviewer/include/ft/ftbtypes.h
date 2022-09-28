/*static char *sccsid="@(#) ftbtypes.h (1.3)  14:21:02 07/08/97";			

   ftbtypes.h -- public typedefs for collection functions		

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------		
  | StM 5.1B 91/06/20 | fgnfid[] size from FTGMXFILT to FTNMXFILT
  | rtk 5.0C 90/06/07 | avoid nested comments				
  | bf	5.0  89/11/06 | Created.					
  |-------------------|---------------------------------		

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */

#ifndef	ftbtyp_h
#define ftbtyp_h
#ifndef	ftgcon_h
#include "ftgconst.h"
#endif

	/* configuration file structure */
typedef struct	_FTFIG	{
	char	fgfuln[FTNPATHSZ];	/* full name of configuration file */
	char	fgpath[FTNPATHSZ];	/* path excluding node,device,file */
	char	fgdev[FTGDEVSZ];	/* device name */
#ifdef	FTHNET
	char	fgnode[FTGNODESZ];	/* node name */
	char	fgnfid[FTNMXFILT];	/* network transport filter */
#else	/*!FTHNET*/
	char	fgnode[1];
	char	fgnfid[1];
#endif	/*!FTHNET*/
}	FTFIG;

	/* function prototypes */
#include "ftgfuncs.h"

#endif	/*!ftbtyp_h*/
