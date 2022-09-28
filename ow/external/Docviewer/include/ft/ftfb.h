/*static char *sccsid="@(#) ftfb.h (1.3)  14:21:03 07/08/97";			

   ftfb.h  - typedefs for filters					

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------		
  | swr 5.1B 91/05/27 | 91012901: added #ifdef FTCPP (C++ safe links)
  | bf  5.0W 91/02/01 | add longs parameter to ttopen function; include ftutypes
  | bf  5.0W 90/10/18 | restructure FTFB for filtmem.dn, based pointers
  | bf  5.0D 90/08/02 | add ftfb_bh					
  | rtk 5.0C 90/06/07 | avoid nested comments				
  | ah  5.0  90/05/18 | added comments					
  | bf  5.0  89/11/07 | include fthandle.h instead of ftgtypes.h	
  | DG	5.0  89/08/18 | include ftgtypes.h FTHNET or not		
  | DG	5.0  89/08/17 | add ftfb_apih to FTFFB				
  | bf  4.6  89/03/20 | rename FTHNET fields				
  | cs	4.6  88/12/23 | FTFFB elements ftfb_poff & ftfb_appl now FAR	
  | DG	4.6  88/12/22 | move FTNTAB to ftgtypes.h			
  | rtk 4.6  88/12/07 | add FTFFB					
  |-------------------|---------------------------------		

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */

#ifndef ftfb_h
#ifndef	fthndl_h
#include "fthandle.h"		/* for FTNFH, FTBH, FTAPIH */
#endif
#ifndef ftutype_h
#include "ftutypes.h"		/* for FTFATR */
#endif
#define ftfb_h  1

typedef long	FTFT;		/* ptr to area used by ftftell, ftfseek */

#ifdef	FTCPP
extern	"C" {
#endif
	/* FTFB structure for management of filter streams */
typedef struct _FTFB	FTFB;
struct  _FTFB {
	FTAPIH		ftfb_apih;
#ifdef	FTHNET
	/* these elements only used if file is remote */
	long		ftfb_rfh;	/* remote FTFB handle */
	FTNFH		ftfb_nfh;	/* virtual circuit connection */
#endif
	UCHAR FTBASED((_segment)_self)	*ftfb_ptr;	/* current char */
	UCHAR FTBASED((_segment)_self)	*ftfb_base;	/* buffer address */
	char FTBASED((_segment)_self)	*ftfb_appl;	/* private data */
	FTFT FTBASED((_segment)_self)	*ftfb_poff;	/* current state */
	FTFT FTBASED((_segment)_self)	*ftfb_boff;	/* state at buf start */
	char FTBASED((_segment)_self)	*ftfb_edit;	/* editor name */
	FTFB FAR			*ftfb_next;	/* next FTFB in chain */

		/* pointers to the filter functions */
	int (PASCAL		*ftfb_close) FTPL((FTFB FAR *));
	int (FTLOADDS PASCAL	*ftfb_fbuf) FTPL((FTFB FAR *));
	int (FTLOADDS PASCAL	*ftfb_pbuf) FTPL((int, FTFB FAR *));
	int (PASCAL		*ftfb_tell) FTPL((FTFB FAR *, FTFT FAR *));
	int (PASCAL		*ftfb_seek) FTPL((FTFB FAR *, FTFT FAR *));
	int (FTLOADDS PASCAL	*ftfb_latr) FTPL((FTFB FAR *, char FAR *,
	    char FAR *, int, FTFATR FAR *));

	unsigned	ftfb_tsize;	/* size of info returned by ftftell */
	unsigned	ftfb_asize;	/* size of FTFB+ memory block */
	short		ftfb_cnt;	/* bytes remaining in buffer */
	short		ftfb_bsiz;	/* buffer length */
	short		ftfb_file;	/* rfd set by standard filter */
	USHORT		ftfb_flag;	/* see ftfconst.h */
	USHORT		ftfb_flg2;	/* filter-specific */
	FTBH		ftfb_bh;	/* for ftbfopen() */
};

	/* filter table entry */
typedef struct _FTFILTT {
		/* filter id */
	CONST char     *ttfid;

		/* filter open function */
	int	(PASCAL *ttopen) FTPL((char FAR *, char FAR *, FTFB FAR *,
		    long FAR *));

		/* filter information function */
	int	(PASCAL *ttinfo) FTPL((FTAPIH, char FAR *, char FAR *,
		    long FAR *));
} FTFILTT;
#ifdef	FTCPP
}
#endif

/* pull in the function prototypes requiring FTFB definition */
#include "ftffuncs.h"

#endif  /* ftfb_h */
