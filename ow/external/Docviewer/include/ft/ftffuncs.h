/*static char *sccsid="@(#) ftffuncs.h (1.3)  14:21:04 07/08/97";			

   ftffuncs.h -- function prototypes for document text filters		

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------		
  | hjb 5.1B 91/10/01 | add ftfputs()
  | cps 5.1B 91/09/05 | ftmfseek, ftmftell take FAR ftft
  | PW  5.1B 91/07/05 | new prototypes for ftdtrd, ftdtwr 
  | hjb 5.1B 91/06/25 | ftdtrd(),ftdtwr()
  | bf  5.1A 91/05/15 | ftmfpbuf: unsigned -> int
  | bf  5.0E 91/04/11 | FTHANSIC: ftfprntf prototype
  | cps 5.1A 91/02/27 | add ftqhostr
  | bf  5.0W 91/02/20 | add lib parameter to ftfsdopn
  | bf  5.0W 91/02/04 | ftfhop: add longs,nlongs parameters
  | hjb	5.0W 91/02/01 | change ftfoopen() prototype (add ftapih)
  | hjb	5.0W 91/01/28 | DLL repackaging: ftfubop,ftfufop -> ftfbop,ftffop
  | bf  5.0W 91/01/14 | include ftfgfunc instead of ftgfuncs; add ftmfxsio
  | bf  5.0W 91/01/07 | ftfubop: FAR parameters
  | hjb	5.0W 91/01/03 | FTAPIFUN ftfdper(),ftfbinit(),ftfbchk() (for ftpit)
  | bf  5.0W 91/01/02 | add ftuskws prototype [FTAPIFUN - hjb]
  | hjb	5.0W 90/12/20 | ftfxsxit is now NULL outside the API
  | hjb	5.0W 90/12/19 | ftfopen(),ftfsopen() (MACROS) moved here from ftftypes.h
  | bf  5.0W 90/12/12 | drop FTFFB, change some parameter lists
  | hjb	5.0W 90/12/05 | exit function prototypes; ftfprntf(ftapih,...)
  | hjb	5.0W 90/11/26 | DLL repackaging
  | rtk 5.0C 90/06/07 | avoid nested comments				
  | KRL 5.0C 90/05/29 | ftmfpbuf(unsigned, ... );			
  | bf  5.0  89/11/09 | test fthndl_h instead of ftgtyp_h; drop FTPD,FTFL
  | DG	5.0  89/10/03 | add ftihflin(),ftffmemo; change ftufcs(),ftuscs()
  | bf  4.6  89/04/04 | FTPL						
  | DG	4.6  89/02/10 | add ftfxaopn()					
  |-------------------|---------------------------------		

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */

/* WHERE POSSIBLE, PLEASE MAINTAIN IN ALPHABETICAL ORDER BY FUNCTION NAME */

	/* the function prototypes in this file require ftfb.h
	   but not fthandle.h; the rest are in ftfgfunc.h */

extern int FTAPIFUN	ftdtrd FTPL((FTAPIH, FTBH, FTFB FAR * FAR *,
			    FTTIME FAR *, int));
extern int FTAPIFUN	ftdtwr FTPL((FTAPIH, FTBH, FTFB FAR * FAR *));
extern int FTAPIFUN	ftfbal FTPL((FTAPIH, unsigned, unsigned, unsigned,
			    unsigned, FTFB FAR * FAR *));
extern int FTAPIFUN	ftfbchk FTPL((FTFB FAR *));
extern void FTAPIFUN	ftfbfr FTPL((FTFB FAR *));
extern int FTAPIFUN	ftfbin FTPL((FTAPIH, unsigned, unsigned, unsigned,
			    unsigned, unsigned, char FAR *));
extern FTFT FAR * FTAPIFUN
			ftfbinit FTPL((FTFB FAR *));
extern FTFB FAR * FTAPIFUN
			ftfdopen FTPL((FTAPIH, int, char FAR *));
extern int FTAPIFUN	ftfdper FTPL((FTFB FAR *, long));
extern int FTAPIFUN	ftfmemo FTPL((FTAPIH, char FAR *, int,
			    FTFB FAR * FAR *));
extern FTFB FAR *	ftfoopen FTPL((FTAPIH, char FAR *, char FAR *, int));
#define ftfopen(f,l)	ftfopn(ftapih,f,l)
extern FTFB FAR * FTAPIFUN
			ftfopn FTPL((FTAPIH, char FAR *, char FAR *));
extern int FTAPIFUN	ftfclose FTPL((FTFB FAR *));
extern int FTAPIFUN	ftfhlk FTPL((FTAPIH, FTFH, FTFB FAR * FAR *));
extern int FTAPIFUN	ftfhop FTPL((FTAPIH, char FAR *, char FAR *, long FAR *,
			    int, FTFB FAR * FAR *));
extern int FTAPIFUN	ftfhun FTPL((FTAPIH, FTFB FAR *, FTFH FAR *));
extern void PASCAL	ftfpbchk FTPL((int, FTFB FAR *, int));
extern int FTAPIFUN	ftfpbuf FTPL((FTFB FAR *));
extern FTFT FAR * PASCAL
			ftfpbint FTPL((FTFB FAR *));
#ifdef	FTHANSIC
extern int CDECL FTLOADDS FTEXPORT
			ftfprntf(FTAPIH, FTFB FAR *, char FAR *, ...);
#else
#ifndef	vms
/*VARARGS3*/
extern int CDECL FTLOADDS FTEXPORT
			ftfprntf FTPL((FTAPIH, FTFB FAR *, char FAR *, ...));
#endif
#endif
extern int FTAPIFUN	ftfputs FTPL((CONST char FAR *, FTFB FAR *));
extern FTFB FAR * FTAPIFUN
			ftfsdopn FTPL((FTAPIH, int, int, char FAR *, int));
extern int FTAPIFUN	ftfseek FTPL((FTFB FAR *, FTFT FAR *));
#define ftfsopen(f,l)	ftfsopn(ftapih,f,l)
extern FTFB FAR * FTAPIFUN
			ftfsopn FTPL((FTAPIH, char FAR *, char FAR *));
#define ftfsthaw()
extern int FTAPIFUN	ftfstro FTPL((FTAPIH, char FAR *, FTFB FAR * FAR *));
extern int FTAPIFUN	ftftell FTPL((FTFB FAR *, FTFT FAR *));
#define ftfthaw()
extern int FTAPIFUN	ftftsize FTPL((FTFB FAR *));
#define	ftfubop(p,f)			ftfbop(ftapih,p,f)
extern FTFB FAR * FTAPIFUN
			ftfbop FTPL((FTAPIH, char FAR *, char FAR *));
#define	ftfufop(p,t,x,f)		ftffop(ftapih,p,t,x,f)
extern FTFB FAR * FTAPIFUN
			ftffop FTPL((FTAPIH, FTFB FAR *, int,
			    int (FTLOADDS PASCAL FAR *) FTPL((FTAPIH, int, int,
			    char FAR *, char FAR *, int)),
			   int));
extern FTFB FAR * FTAPIFUN
			ftfuopen FTPL((FTFB FAR *, FTBH, FTSH, int, int));
#define ftfxaopn(f,p,e,z,c,l) ftfxao(ftapih,f,p,e,z,c,l)
extern int FTAPIFUN 	ftfxao FTPL((FTAPIH, int, FTFB FAR *, int, int, int,
			    int));
#define	ftfxsxit	NULL
#define	ftfxtopn(f,t,n,x,l) ftfxto(ftapih,f,t,n,x,l)
extern int FTAPIFUN	ftfxto FTPL((FTAPIH, int, int, FTFB FAR *,
			    int (PASCAL FAR *) FTPL((FTAPIH, int, int,
			    char FAR *, char FAR *, int)),
			   int));
extern long PASCAL	ftihflin FTPL((FTFB FAR *));
extern int PASCAL		ftmfclos FTPL((FTFB FAR *));
extern int FTLOADDS PASCAL	ftmfgbuf FTPL((FTFB FAR *));
extern int FTLOADDS PASCAL	ftmfpbuf FTPL((int, FTFB FAR *));
extern int PASCAL		ftmfseek FTPL((FTFB FAR *, FTFT FAR *));
extern int PASCAL		ftmftell FTPL((FTFB FAR *, FTFT FAR *));
extern int PASCAL	ftmfxsio FTPL((FTAPIH, FTMFFH, unsigned, char FAR *,
			    short FAR *, unsigned, FTFB FAR * FAR *));
extern FTFB FAR * FTAPIFUN
			ftqhostr FTPL((FTAPIH, char FAR *));
extern int FTAPIFUN	ftucseq FTPL((FTFB FAR *, unsigned FAR *, int, int,
			    char FAR *, int));
extern int FTAPIFUN	ftufcs FTPL((FTFB FAR *, char FAR *, int FAR *));
extern int FTAPIFUN	ftufis FTPL((FTFB FAR *, int FAR *));
extern long FTAPIFUN	ftufls FTPL((FTFB FAR *, int FAR *));
extern int FTAPIFUN	ftuscs FTPL((FTFB FAR *, char FAR *, int FAR *));
extern int FTAPIFUN	ftuskws FTPL((FTFB FAR *, int FAR *, int FAR *));

	/* prototypes which require other header files */
#include "ftfgfunc.h"
