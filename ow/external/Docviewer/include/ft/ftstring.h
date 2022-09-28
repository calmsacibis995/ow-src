/*static char *sccsid="@(#) ftstring.h (1.1)  14:21:08 07/08/97";

   ftstring.h -- prototypes & defines for C-runtime string, memory functions

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------
  | wtr 5.1A 91/05/14 | Get rid of nested comments
  | DG  5.1A 91/05/14 | add STRCAT, STRNCPY
  | bf  5.1A 91/04/16 | add STRNCMP
  | bf  5.1W 91/03/12 | Created
  |-------------------|---------------------------------

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.	*/

#ifdef	FTHMSDOS
#include <string.h>

#define FTUCSTR		ftucfstr
#define FTURINDX	_fstrrchr
#define FTUINDX		_fstrchr

#define MEMCPY		_fmemcpy
#define MEMSET		_fmemset
#define STRCMP		_fstrcmp
#define STRCPY		_fstrcpy
#define STRCAT		_fstrcat
#define STRLEN		_fstrlen
#define	STRNCMP		_fstrncmp
#define	STRNCPY		_fstrncpy

#else	/*!FTHMSDOS*/

#define FTUCSTR		ftucstr
#define FTURINDX	fturindx
#define FTUINDX		ftuindx

#define MEMCPY		memcpy
#define MEMSET		memset
#define STRCMP		strcmp
#define STRCPY		strcpy
#define STRCAT		strcat
#define STRLEN		strlen
#define	STRNCMP		strncmp
#define	STRNCPY		strncpy

#endif	/*!FTHMSDOS*/
