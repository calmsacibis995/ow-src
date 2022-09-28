/*static char *sccsid="@(#) ftutypes.h (1.3)  14:21:09 07/08/97";			

   ftutypes.h -- public typedefs for utility functions			

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------		
  | bf  5.0W 91/01/11 | move prototypes to ftgfuncs.h
  | hjb	5.0W 90/12/03 | ftutw()'s callback function is FTLOADDS
  | hjb	5.0W 90/11/27 | DLL repackaging; move ftutw() from fthandle.h
  | DAT 5.0D 90/08/11 | SM: VM port					
  | rtk 5.0C 90/06/07 | avoid nested comments				
  | KRL 5.0  89/11/11 | ftudatr() ftufach() ftufatr() from fthio.h	
  | bf	5.0  89/11/06 | Created.					
  |-------------------|---------------------------------		

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */

#ifndef	ftutyp_h
#define ftutyp_h
#ifndef	ftgcon_h
#include "ftgconst.h"
#endif
#ifndef	fthndl_h
#include "fthandle.h"
#endif

	/* device for ftufatr(), ftudatr() */
typedef struct _FTDEV	{
	int	dev;
}	FTDEV;

	/* file attributes from ftufatr() */
typedef	struct	_FTFATR	{
	long	ftufsize;		/* file length in bytes */
#ifdef	FTHUNIX
	long	ftufino;		/* inode number */
#endif
	FTTIME	ftufdate;		/* last modified date/time */
	FTTIME	ftufctime;		/* time of file's last status change */
	short	ftufmode;		/* file mode */
	USHORT	ftufuid;		/* file owner's user id */
	USHORT	ftufgid;		/* file owner's group id */
	FTDEV	ftufdev;		/* device */
}	FTFATR;

	/* bit code structure */
typedef struct 	_FTBCODE	{
	long  bit;	/* bit value */
	char *str;	/* ASCII code for corresponding bit value */
} 	FTBCODE;

typedef struct 	_FTDATR	{
	long		ftudfree;		/* total device free bytes  */
	long		ftudbpc;		/* bytes per cluster	    */
	unsigned long	ftudinod;		/* number of free inodes    */
	int		ftudmode;		/* success flags	    */
#ifdef	FTHMSDOS
	char		ftudname[FTGDEVSZ];	/* filsys name		    */
#else
	char		ftudname[FTGPATHSZ];	/* filsys name		    */
#endif
}	FTDATR;

	/* function prototypes */
#include "ftgfuncs.h"

#endif	/*!ftutyp_h*/
