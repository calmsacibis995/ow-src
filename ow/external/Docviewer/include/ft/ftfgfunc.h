/*static char *sccsid="@(#) ftfgfunc.h (1.3)  14:21:04 07/08/97";

   ftfgfunc.h -- prototypes which need both ftgtypes.h and ftfb.h

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------
  | swr	5.1B 91/12/18 | 91071804: remove ifdef FTSVSS around ftmfsopn
  | DG	5.1B 91/09/11 | add ftcsmopn()
  | DG	5.1B 91/07/05 | ftmfsopn: PASCAL -> FTAPIFUN
  | swr 5.1B 91/05/27 | 91012901: added #ifdef FTCPP (C++ safe links)
  | bf  5.1A 91/05/15 | ftmfsopn: FTAPIFUN -> PASCAL
  | cps 5.1A 91/02/13 | Function prototype for ftmfsopn (ifdef'd for FTSVSS)
  | bf  5.0W 90/12/19 | FTAPIFUN ftfuopen
  | bf  5.0W 90/12/12 | drop ftmfsopn, fix ftfuopen (FTFFB -> FTFB)
  | hjb	5.0W 90/11/30 | correct prototype for ftblop()
  | hjb	5.0W 90/11/12 | DLL repackaging (ftbfatr)
  | DG	5.0B 90/02/21 | ftfsffbf -> ftfsfgbf
  | DG  5.0  89/10/31 | some catalog stream functions moved from ftgfuncs.h
  | DG	5.0  89/10/04 | ftfsffbf(), ftfsfsek(), ftfsftel()
  | DG	5.0  89/08/18 | ftfsfdop(), ftfsfopn()
  | bf	4.6  89/04/05 | Created.
  |-------------------|---------------------------------

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */


/* WHERE POSSIBLE, PLEASE MAINTAIN IN ALPHABETICAL ORDER BY FUNCTION NAME */

	/* the prototypes in this file require ftfb.h and other headers */
	/* both ftgfuncs.h and ftffuncs.h include this file */

#ifdef	FTCPP
extern	"C" {
#endif
#ifdef	ftctyp_h
#define	ftbfopen(b,c)	ftbfop(ftapih,b,c)
extern FTFB FAR * FTAPIFUN
			ftbfop FTPL((FTAPIH, FTBH, FTCINFO FAR *));
extern int PASCAL	ftcsmopn FTPL((FTAPIH, FTCH, FTFB FAR * FAR *,
			FTCINFO FAR *, int, int, FTFID FAR *, int));
extern int FTAPIFUN	ftcsread FTPL((FTAPIH, FTCH, FTFB FAR * FAR *,
			    FTCINFO FAR *, int, int, FTFID FAR *, int));
extern int FTAPIFUN	ftcsrwrt FTPL((FTAPIH, FTCH, FTCINFO FAR *,
			    FTFB FAR * FAR *, int));
extern int FTAPIFUN	ftcssdup FTPL((FTAPIH, FTCH, FTFB FAR * FAR *,
			    FTCINFO FAR *, FTCID, FTCINFO FAR *, int, int,
			    FTFID FAR *, int));
extern int FTAPIFUN	ftcswrt FTPL((FTAPIH, FTCH, FTFB FAR * FAR *,
			    FTCINFO FAR *));

#ifdef	ftutyp_h
#define	ftbfatr(b,c,f,l,a) ftbfgi(ftapih,b,c,f,l,a)
extern int FTAPIFUN	ftbfgi FTPL((FTAPIH, FTBH, FTCINFO FAR *, FTFB FAR *,
			    int, FTFATR FAR *));
#endif	/*ftctyp_h*/
#endif	/*ftutyp_h*/

#ifdef	ftbtyp_h
#define	ftbfblog(f)	ftblop(ftapih,f)
extern FTFB FAR * FTAPIFUN
			ftblop FTPL((FTAPIH, FTFIG FAR *));
#endif	/*ftbtyp_h*/

extern FTFB FAR * FTAPIFUN
			ftmfsopn FTPL((FTMFFH, FTMFOH, unsigned, char FAR *));
#ifdef	FTCPP
}
#endif
