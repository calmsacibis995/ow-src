/*static char *sccsid="@(#) ftctypes.h (1.3)  14:21:03 07/08/97";			

   ftctypes.h -- public typedefs for catalog functions			

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------		
  | hjb	5.0W 90/11/05 | FTCINFO -> far memory
  | rtk 5.0C 90/06/07 | avoid nested comments				
  | bf	5.0  89/11/06 | Created.					
  |-------------------|---------------------------------		

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */

#ifndef	ftctyp_h
#define ftctyp_h
#ifndef	ftgcon_h
#include "ftgconst.h"
#endif

	/* catalog formatting fields */
typedef struct	_FTCFLDS	{
	FTVCC		cfvirto;	/* virt offset at start of field */
	char	FAR	*cfbuf;		/* ptr to caller's buffer */
	short		cffmtcd;	/* format code */
	USHORT		cffldln;	/* field length */
	short		cfdspln;	/* display length */
	FTFID		cffldid;	/* field id */
	short		cfdsseq;	/* seq no for repeating field id */
}	FTCFLDS;

	/* subdocument descriptor */
typedef	struct	_FTCSDOC	{
	FTTIME		csmtime;	/* modified time */
	FTFID		cstxtid;	/* field id for sub-document text */
	short		csflags;	/* sub-doc flags */
	char		csfilnm[FTNPATHSZ];	/* sub-document file name */
	char		csstore[FTGMXFILT];	/* storage code */
}	FTCSDOC;

	/* catalog entry descriptor */ 
typedef	struct	_FTCINFO	{
	FTTIME		cictime;	/* catalog entry timestamp */
	FTCFLDS FAR	*cicflds;	/* ptr to catalog fields array */
	FTCSDOC FAR	*cicsdoc;	/* ptr to sub-document array */
	FTCID		cicid;		/* catalog id */
	short		cistatus;	/* catalog entry status flags */
	short		cinflds;	/* number of catalog fields */
	short		cinsdocs;	/* number of sub-documents */
}	FTCINFO;

	/* catalog entry status descriptor */
typedef struct _FTCEST	{
	FTCID		cecid;		/* catalog id */
	short		cestatus;	/* catalog entry status flags */
}	FTCEST;

	/* function prototypes */
#include "ftgfuncs.h"

#endif	/*!ftctyp_h*/
