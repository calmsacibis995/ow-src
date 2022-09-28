/*static char *sccsid="@(#) ftnfuncs.h (1.3)  14:21:08 07/08/97";

   ftnfuncs.h -- function prototypes for network filter layer

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------
  | swr 5.1B 91/05/27 | 91012901: added #ifdef FTCPP (C++ safe links)
  | hjb	5.0W 91/02/01 | add FTAPIH parameter (except ftnfnfb())
  | hjb	5.0W 90/12/11 | FTAPIFUN, FAR argument pointers
  | bf  4.6  89/04/04 | FTPL
  | bf  4.6  89/02/09 | Split from ftnfb.h

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */


	/* network filter stream functions */
#ifdef	FTCPP
extern	"C" {
#endif
extern FTNFH FTAPIFUN	ftnopen FTPL((FTAPIH, char FAR *, char FAR *));
extern FTNFH FTAPIFUN	ftnserve FTPL((FTAPIH, char FAR *));
extern FTNFB	*ftngnfb FTPL((FTAPIH, char FAR *, char FAR *, int (*)(),
			    int (*)(), int (*)(), unsigned, int));
extern int	ftnfnfb FTPL((FTNFB *));
extern FTNFILTT	* FTAPIFUN ftnfsrch FTPL((char FAR *));
#ifdef	FTCPP
}
#endif
