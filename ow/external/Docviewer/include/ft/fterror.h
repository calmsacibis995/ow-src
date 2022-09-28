/*static char *sccsid="@(#) fterror.h (1.3)  14:21:03 07/08/97";			

   fterror.h -- error codes set by pi functions				

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------		
  | swr 5.1B 91/11/14 | 91082703: add FTESBUSY
  | MM  5.1B 91/09/26 | FTEFTFR
  | PW  5.1B 91/06/07 | FTENODTD
  | swr 5.1B 91/05/27 | 91012901: added #ifdef FTCPP (C++ safe links)
  | cps 5.1A 91/04/11 | Add FTEELOOP
  | cps 5.1A 91/04/08 | Add FTENOSR
  | wr  5.1  91/02/12 | Add FTENOENTRY & FTEBFAIL
  | bf  5.0W 90/12/22 | FTDLLIB: don't reference ftonret
  | bf  5.0W 90/12/19 | FTDLFAR -> FAR
  | hjb	5.0W 90/11/23 | declare ftegerr(), etc. as FTAPIFUN for DLL
  | ah  5.0D 90/08/27 | add comments for FTENOTYET & FTEOLDSER		
  | bf  5.0D 90/08/02 | add FTENOTYET					
  | bf  5.0D 90/07/31 | add FTEOLDSER					
  | ah  5.0D 90/06/28 | remove FTEBIGCAT				
  | rtk 5.0C 90/06/07 | avoid nested comments				
  | DG	5.0C 90/04/17 | drop pre-5.0 section and fterrstr		
  | DG	5.0  89/11/24 | FTENOQRY, FTENODOC				
  | KRL 5.0  89/11/22 | FTEFILTL					
  | bf  5.0  89/11/09 | test for fthndl_h instead of ftgtyp_h		
  | ah  5.0  89/11/01 | FTENSMETA					
  | cs  5.0  89/10/06 | FTEBADMF					
  | SM  5.0  89/09/21 | FTEPDESC					
  | DG	5.0  89/08/08 | FTENOSRV					
  | AH	5.0  89/08/03 | FTECATOLD & FTECATNEW				
  | DG	5.0  89/07/06 | API repackaging					
  | KRL 5.0  89/06/28 | #define fterr_h without value (fixes "fterrno 1")
  | cs  4.6  89/05/15 | FTENOWGHT					
  | DG	4.6  89/04/24 | MS-Windows: FTNOD for DLL NODATA modules	
  | ah  4.6  89/04/21 | FTEOSTRM					
  | ah  4.6  89/04/17 | FTEBADFID and FTECATFMT				
  | rtk 4.6  89/04/12 | FTEBADDYX					
  | bf  4.6  89/01/05 | FTEHANDLE					
  | bf  4.6  88/12/20 | FTEBADLK					
  | DG	4.6  88/11/28 | FTDLUSR						
  |-------------------|---------------------------------		

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */

#ifndef fterr_h
#define fterr_h

#ifndef fthio_h
#include "fthio.h"
#endif

#ifndef	FTNOD
extern int	ftvbose, ftdebug;
#endif	/*!FTNOD*/

	/* definition of structure for saving/restoring error variables */
typedef struct _FTERRSAVE {
	int	serrno;
	int	serrsub;
	FTWHO	serrwho;
} FTERRSAVE;

#ifndef	ftapi_h

#ifdef	fthndl_h
	/* prototypes for functions that manipulate error diagnostics */
	/* fthandle.h required for these */
#ifdef	FTCPP
extern	"C" {
#endif
extern int FTAPIFUN	ftegerr FTPL((FTAPIH));
extern int FTAPIFUN	ftegsub FTPL((FTAPIH));
extern FTWHO FTAPIFUN	ftegwho FTPL((FTAPIH));
extern void FTAPIFUN	ftegsav FTPD((FTAPIH, FTERRSAVE FAR *));
extern int FTAPIFUN	fteserr FTPL((FTAPIH, int, FTWHO));
extern FTWHO FTAPIFUN	fteswho FTPL((FTAPIH, FTWHO));
extern int FTAPIFUN	ftesall FTPL((FTAPIH, int, int, FTWHO));
extern void FTAPIFUN	ftessav FTPD((FTAPIH, FTERRSAVE FAR *));
extern int FTAPIFUN	ftecall FTPL((FTAPIH));
#ifdef	FTCPP
}
#endif

	/* macros for backwards compatibility */
#define	fterrno		ftegerr(ftapih)
#define	fterrsub	ftegsub(ftapih)
#define	fterrwho	ftegwho(ftapih)
#define	ftseterr(e)	fteserr(ftapih,(e),who)
#define	ftsetall(e,s)	ftesall(ftapih,(e),(s),who)
#define	ftsetwho()	fteswho(ftapih,who)
#define	ftclrerr()	ftecall(ftapih)
#define	ftsaverr(p)	ftegsav(ftapih,(p))
#define	ftreserr(p)	ftessav(ftapih,(p))
#endif	/*fthndl_h*/

#endif	/*!ftapi_h*/

	/* macros to return an error */
	/* ftonret() is a macro defined in fthio.h */
#ifdef	FTHINS
#define ftreterr(v,e)	return(ftonret(), ftseterr(e), (v))
#define ftretall(v,e,s)	return(ftonret(), ftsetall(e,s), (v))
#else	/*!FTHINS*/
#ifdef	FTDEBUG
#ifndef	FTDLLIB
#define ftreterr(v,e)	return(ftonret(), ftseterr(e), (v))
#define ftretall(v,e,s)	return(ftonret(), ftsetall(e,s), (v))
#endif
#endif	/*FTDEBUG*/
#ifndef	ftreterr
#define ftreterr(v,e)	return(ftseterr(e), (v))
#define ftretall(v,e,s)	return(ftsetall(e,s), (v))
#endif	/*!ftreterr*/
#endif	/*!FTHINS*/

	/* FTUCORE maps to nothing unless debug turned on */
#ifdef	FTDEBUG
#define	FTUCORE(who)	ftucore(who)
#else
#define	FTUCORE(who)
#endif

/* Note that new fterrno values should also be added to ftpit/fttdefs.c. */

#define FTEDBXIST	1
#define FTESYSTEM	2
#define FTEFLXIST	3
#define FTEOPENO	4
#define FTEOPENI	5
#define FTEREADF	6
#define FTEWRITEF	7
#define FTEBADARG	8
#define FTENOTDB	9
#define FTENOFILE	10
#define FTEUNDEF	11
#define FTESYNTAX	12
#define FTETOOBIG	13
#define FTENOMEM	14
#define FTEACCESS	15
#define FTETOOMNY	16
#define FTEOCATI	17
#define FTEOCATO	18
#define FTEONDXI	19
#define FTEONDXO	20
#define FTEDBBUSY	21
#define FTEOF		22
#define FTELOCKED	23
#define FTENOLOCK	24
#define FTEIONO 	25
#define FTEXFSZ 	26
#define FTENOSRCH	27
#define FTEBADCAT	28
#define FTESTREAM	29


#define FTESEEKF	32


#define FTECXDEL	35
#define FTECATOVFL	36
#define FTECANCEL	37
#define FTECHILD	38
#define FTEBADMSG	39
#define FTENOTCAT	40
#define FTENOTCIX	41
#define FTENOSPC	42
#define FTEFLTYPE	43
#define FTECATEND	44
#define FTEOR		45
#define FTESEARCH	46
#define FTEEXEC 	47
#define FTEUPDNDX	48
#define FTEBADVER	49
#define FTEUNLINK	50
#define FTEDEADLK	51
#define FTEBADNDX	52
#define FTEINTNDX	53
#define FTEBADSR	54
#define FTENOSTR	55
#define FTEOLDHAT	56
#define FTELEMMA	57
#define FTEINTSSB	58
#define FTEBADFR	59
#define FTEBADSTATE	60
#define FTEWINDOWS	61
#define FTENOSUP	62
#define	FTENOINIT	63
#define FTENFSLK	64
#define FTENETIO	65
#define FTEBADLK	66
#define FTEHANDLE	67
#define FTENOMARK	68
#define FTEBADDYX	69
#define FTEBADFID	70 	/* Bad field id (FID) */
#define FTECATFMT	71	/* Bad data in catalog write stream */
#define FTEOSTRM	72	/* Function not allowed due to open stream */
#define	FTENOWGHT	73	/* No weights available (ftsgwght()) */
#define FTECATOLD	74	/* Catalog format is too old for operation */
#define FTECATNEW	75	/* Catalog format is newer than software */
#define	FTENOSRV	76	/* no service because of ftads.h */
#define FTEPDESC	77	/* improper use of pstruct functions */
#define	FTEBADMF	78	/* Wrong version of minifile */
#define FTENSMETA	79	/* Operation not allowed on a meta-collection */
#define	FTEFILTL	80	/* invalid filter list */
#define	FTENOQRY	81	/* empty query tree (ftshresm) */
#define	FTENODOC	82	/* no documents found (ftshresm) */
#define FTEOLDSER	83	/* server will not support operation */
#define FTENOTYET	84	/* bh close attempted when related handles */
				/* have not been closed yet. */
#define FTENOENTRY      85      /* failed to find a given entry point within */
                                /* dynamically bound library                 */
#define FTEBFAIL        86      /* failed to bind given library */
#define	FTENOSR		87	/* No search result exists */
#define	FTEELOOP	88	/* Endless loop */
#define	FTENODTD	89	/* no DTD entry in catalog */
#define	FTEFTFR		90	/* FTFR handles still in use ftapitrm failed */
#define	FTESBUSY	91	/* Index blocked by busy search process */

#endif /* fterr_h */
