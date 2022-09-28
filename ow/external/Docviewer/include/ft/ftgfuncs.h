/*static char *sccsid="@(#) ftgfuncs.h (1.4)  14:21:06 07/08/97";			

   ftgfuncs.h - extern function declarations for parameter checking	

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------		
  | rdc 5.1B 91/09/11 | add ftfxco()
  | DG	5.1B 91/09/03 | move ftufulfn prototype here from fthandle.h
  | hjb 5.1B 91/07/23 | changed FTBICALL to FTAPIFXW
  | swr 5.1B 91/05/27 | 91012901: added #ifdef FTCPP (C++ safe links)
  | bf  5.1A 91/04/15 | FTBICALL ftbiresm
  | hjb	5.1W 91/03/22 | ftbiresm not in DLL
  | hjb	5.0W 91/01/18 | add ftapih to ftnfind() calling sequence
  | bf  5.0W 91/01/11 | move prototypes from other headers; include ftfgfunc
  | hjb	5.0W 91/01/03 | FTAPIFUN ftufigs(),ftusfig() (for ftpit)
  | bf  5.0W 90/12/12 | drop ftfsf*, ftmfsopn; fix ftmfxsio prototype
  | hjb	5.0W 90/12/06 | ftnfind() prototype change
  | hjb	5.0W 90/11/30 | DLL repackaging
  | rtk 5.0C 90/06/07 | avoid nested comments				
  | DG	5.0C 90/03/08 | ftshcfld replaces ftfxcxtr			
  | DG	5.0B 90/02/21 | ftfsffbf -> ftfsfgbf				
  | DG	5.0B 90/01/10 | ftmfxsio					
  | bf  5.0B 90/01/03 | ftshocat					
  | bf  5.0  89/11/03 | ftbindex,ftbistrt,ftshstrt,ftnfgfnd		
  | ah  5.0  89/11/01 | Removed ftcdup() - it is no longer supported.	
  | bf  5.0  89/11/01 | Windows: temp. reset retval of ftbindex to FTIH	
  | SM  5.0  89/11/01 | added argument to ftppfrep()			
  | DG  5.0  89/10/31 | some catalog stream functions moved to ftfgfunc.h
  | ah  5.0  89/10/30 | Catalog stream functions added			
  | DG	5.0  89/10/20 | ftuc?(),ftu?c(),ftu?scnl(),ftusstr(): FTDLFAR->FAR
  | SM  5.0  89/10/06 | add ftppallp(),ftppptoh(),ftpphtop(),ftppfrep()	
  | AH  5.0  89/09/05 | ftcalcsp moved to ftpfuncs.h			
  | KRL 5.0  89/09/05 | ftcalcsp(FTBB, ...)				
  | DG	5.0  89/09/05 | ftsmopen(), ftsmclse()				
  | DG	5.0  89/07/25 | MS-Windows: move ftumnrlc() here from fthio.h	
  | DG	5.0  89/07/12 | API repackaging					
  | DG	5.0  89/06/19 | fix ftugdfs() prototype (FAR -> FTDLFAR)	
  | DG	5.0  89/06/14 | ftudatr(), ftugdfs(): PASCAL; FTDLFAR parameters
  | DG	4.6B 89/05/30 | ftfrsdos()					
  | cs  4.6  89/05/26 | add ftsgkey(), ftsgwght(), ftswrix()		
  | DG	4.6  89/04/24 | ftpminit() returns FTHNDL; add ftpmstat()	
  | DG	4.6  89/04/18 | drop ftumnalc(), ftumnfre() for MS-Windows	
  | DG	4.6  89/04/14 | change ftpmsstk()				
  | cs  4.6  89/04/06 | add ftmfrtel(), ftmfrsek()			
  | cs  4.6  89/04/06 | remove ftmfradd,ftmfrset, change ftmfrget ftmfrput
  | pbc 4.6  89/04/05 | VMS: reinstate ftufatr KLUGE ("..." not yet supp.)
  | DG	4.6  89/02/23 | add ftfxh...(), ftuwslnh()			
  | bf  4.6  89/03/20 | merge ftfuncs.h, ftgfuncs.h; use FTPD,FTPL	
  | cs	4.6  89/02/28 | add minifile routines				
  | DG	4.6  89/01/06 | change ftu?scnl(), ftusstr()			
  | DG	4.6  88/12/22 | change parms ftntstr(); add fttrtget(),fttrtrls()
  | DG	4.6  88/12/20 | add ftuint21 for MSDOS				
  | DG	4.6  88/12/14 | change parameters to date conversion functions	
  | spa 4.6  88/12/13 | change ftfropen() parameters to FAR		
  | rtk 4.6  88/11/21 | ftumfalc, ftumffre moved to ftgmem.h		
  |-------------------|---------------------------------		

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */

/* this file is included by several different header files to get
   function prototypes for public functions */

/* WHERE POSSIBLE, PLEASE MAINTAIN IN ALPHABETICAL ORDER BY FUNCTION NAME */

#ifdef	FTCPP
extern	"C" {
#endif
#ifdef	ftbtyp_h
#define ftbcreate(f,d,p,o,s,n,l,m) ftbhcr(ftapih,f,d,p,o,s,n,l,m)
extern int FTAPIFUN	ftbhcr FTPL((FTAPIH, FTFIG FAR *, char FAR *,
			    char FAR *, char FAR *, char FAR * FAR *, int,
			    long FAR *, int));
#define ftbgparm(f,b,o,c) ftbxgi(ftapih,f,b,o,c) 
extern int FTAPIFUN	ftbxgi FTPL((FTAPIH, FTFIG FAR *, char FAR *,
			    char FAR *, long FAR *));
#define ftbindex(f,t,s,l) ftbhix(ftapih,f,t,s,l)
extern int FTAPIFUN	ftbhix FTPL((FTAPIH, FTFIG FAR *, long, long, int));
#define ftbopen(f,c,i)	ftbhop(ftapih,f,c,i)
extern FTBH FTAPIFUN	ftbhop FTPL((FTAPIH, FTFIG FAR *, char FAR *, 
			    char FAR *));
#define ftbremove(f)	ftbhrm(ftapih,f)
extern int FTAPIFUN	ftbhrm FTPL((FTAPIH, FTFIG FAR *));
#define	ftnfgfnd(c,f)	ftfgsl(ftapih,c,f)
extern int PASCAL	ftfgsl FTPL((FTAPIH, char FAR *, FTFIG FAR *));
#ifdef	FTHNET
extern int		ftnfind FTPL((FTAPIH, char FAR *, FTFIG FAR *));
#endif
#define	ftufgfnd(c,f)	ftfgsh(ftapih,c,f)
extern int FTAPIFUN	ftfgsh FTPL((FTAPIH, const char FAR *, FTFIG FAR *));
#define	ftufgset(f,b)	ftfgrd(ftapih,f,b)
extern int FTAPIFUN	ftfgrd FTPL((FTAPIH, FTFIG FAR *, char FAR *));
extern char FAR * FTAPIFUN
			ftufigs FTPL((FTFIG FAR *, char FAR *));
extern int FTAPIFUN	ftusfig FTPL((char FAR *, FTFIG FAR *));
#endif	/*ftbtyp_h*/

#ifdef	ftctyp_h
#define	ftciinit(c,i,b,l) ftchin(ftapih,c,i,b,l)
extern int FTAPIFUN 	ftchin FTPL((FTAPIH, FTCH, FTCINFO FAR *,
			    char FAR * FAR *, int FAR *));
#define	ftcmget(i,m,k,l) ftcmrd(ftapih,i,m,k,l)
extern int FTAPIFUN 	ftcmrd FTPL((FTAPIH, FTCINFO FAR *, FTCMH, unsigned, 
			    long FAR *));
#define	ftcmopen(c,i,m,a,t) ftcmop(ftapih,c,i,m,a,t)
extern FTCMH FTAPIFUN 	ftcmop FTPL((FTAPIH, FTCH, FTCINFO FAR *, FTCMH,
			    long FAR *, long FAR *));
#define	ftcmvcc(i,m,v,k,l) ftcmgi(ftapih,i,m,v,k,l)
extern int FTAPIFUN 	ftcmgi FTPL((FTAPIH, FTCINFO FAR *, FTCMH, FTVCC,
			    unsigned FAR *, long FAR *));
#define	ftcread(c,i,m)	ftchrd(ftapih,c,i,m)
extern int FTAPIFUN 	ftchrd FTPL((FTAPIH, FTCH, FTCINFO FAR *, int));
#define	ftcrewrt(c,i,o,f) ftchrw(ftapih,c,i,o,f)
extern int FTAPIFUN 	ftchrw FTPL((FTAPIH, FTCH, FTCINFO FAR *,
			    FTCINFO FAR *, int));
#define	ftcsdup(c,i,s,o,l) ftcfrk(ftapih,c,i,s,o,l)
extern int FTAPIFUN 	ftcfrk FTPL((FTAPIH, FTCH, FTCINFO FAR *, FTCID,
			    FTCINFO FAR *, int));
#define	ftcsget(c,e,m)	ftcrgi(ftapih,c,e,m)
extern int FTAPIFUN 	ftcrgi FTPL((FTAPIH, FTCH, FTCEST FAR *, int));
extern int FTAPIFUN 	ftcsrblk FTPL((FTAPIH, FTCH, FTCINFO FAR *, int, int,
			    FTFID FAR *, int, char FAR *, int));
#define	ftcwrt(c,i)	ftchwr(ftapih,c,i)
extern int FTAPIFUN 	ftchwr FTPL((FTAPIH, FTCH, FTCINFO FAR *));
#define	ftfucfmt(c,v,n,s,b,l,q) ftfucf(ftapih,c,v,n,s,b,l,q)
extern int FTAPIFUN	ftfucf FTPL((FTAPIH, FTCINFO FAR *, int FAR *, int,
			    USHORT, char FAR *, int, int));
extern int FTAPIFUN	ftfxco FTPL((FTAPIH, FTSH, FTCID, FTCINFO FAR *, int,
			    int, FTFID FAR *, int, int, int, int, int FAR *));
#define	ftfxcget(b,s,r,c,i,l,f) ftfxgf(ftapih,b,s,r,c,i,l,f)
extern int FTAPIFUN	ftfxgf FTPL((FTAPIH, FTBH, FTSH, FTCID, FTCH,
			    FTCINFO FAR *, int, int FAR *));
#define ftfxdopn(f,b,s,r,c,i,e,z,n,k,l)	ftfxop(ftapih,f,b,s,r,c,i,e,z,n,k,l)
extern int FTAPIFUN	ftfxop FTPL((FTAPIH, int, FTBH, FTSH, FTCID, FTCH,
			    FTCINFO FAR *, int, int, int, int, int FAR *));
#define ftfxpmrg(f,b,s,r,c,i,k,l) ftfxpm(ftapih,f,b,s,r,c,i,k,l)
extern int FTAPIFUN	ftfxpm FTPL((FTAPIH, int, FTBH, FTSH, FTCID, FTCH,
			    FTCINFO FAR *, int, int FAR *));
extern int FTAPIFUN	ftshcfld FTPL((FTAPIH, FTSH, FTCID, FTCINFO FAR *,
			    unsigned, char FAR *, unsigned, unsigned,
			    unsigned FAR *));
extern int FTAPIFUN	ftshocat FTPL((FTAPIH, FTSH, FTCINFO FAR *, FTCID,
			    int FAR *));
#define	ftsocat(s,c,f)	ftshgo(ftapih,s,c,f)
extern int FTAPIFUN	ftshgo FTPL((FTAPIH, FTSH, FTCINFO FAR *, int FAR *));
extern int FTAPIFUN	ftufulfn FTPL((FTAPIH, FTCINFO FAR *, FTBH, char FAR *));
#endif	/*ftctyp_h*/

#ifdef	ftityp_h
extern int FTAPIFXW	ftbiresm FTPL((FTAPIH, FTIH, long, int, FTIDATA FAR *));
#endif	/*ftityp_h*/

#ifdef	ftntyp_h
#define	fttrtget(t)	fttrlk(ftapih,t)
extern int FTAPIFUN	fttrlk FTPL((FTAPIH, FTNTAB FAR *));
#define	fttrtrls(t)	fttrun(ftapih,t)
extern int FTAPIFUN	fttrun FTPL((FTAPIH, FTNTAB FAR *));
#endif	/*ftntyp_h*/

#ifdef	ftotyp_h
extern int FTAPIFUN	ftmfoatr FTPL((FTMFFH, FTMFOH, FTOATR FAR *));
#endif	/*ftotyp_h*/
 
#ifdef	ftstyp_h
#define	ftsseek(s,o)	ftshsk(ftapih,s,o)
extern int FTAPIFUN	ftshsk FTPL((FTAPIH, FTSH, FTSOFF FAR *));
#define	ftstell(s,o)	ftshtl(ftapih,s,o)
extern int FTAPIFUN	ftshtl FTPL((FTAPIH, FTSH, FTSOFF FAR *));
extern int FTAPIFUN	ftshresm FTPL((FTAPIH, FTSH, long, int, FTSDATA FAR *));
#endif	/*ftstyp_h*/

#ifdef	ftutyp_h
#define	ftudatr(devp,f,datrp) ftudgatr(ftapih,devp,f,datrp)
extern int FTAPIFUN	ftudgatr FTPL((FTAPIH, FTDEV FAR *, int,
			    FTDATR FAR *));
#ifdef	FTHVMCMS
#define ftufatr(fn,flags,fatrp,rfd,par) ftufgatr(ftapih,fn,rfd,flags,fatrp,par)
extern int PASCAL	ftufgatr FTPL((FTAPIH, char FAR *, int,
			    unsigned, FTFATR FAR *, char FAR *));
#else	/*!FTHMVCMS*/
#define	ftufatr(fn,flags,fatrp,rfd)	ftufgatr(ftapih,fn,rfd,flags,fatrp)
extern int FTAPIFUN	ftufgatr FTPL((FTAPIH, char FAR *, int,
			    unsigned, FTFATR FAR *));
#endif	/*!FTHVMCMS*/
#define	ftufach(fn,flags,fatrp,rfd) ftufsatr(ftapih,fn,flags,fatrp,rfd)
extern int FTAPIFUN	ftufsatr FTPL((FTAPIH, char FAR *, int, FTFATR FAR *,
			    int));
#define	ftutw(p,m,f,u)	ftuttw(ftapih,p,m,f,u)
extern int FTAPIFUN	ftuttw FTPL((FTAPIH, char FAR *, int,
			   int (PASCAL FTLOADDS *) FTPL((FTAPIH, char FAR *,
			   char FAR *, FTFATR FAR *)),
			  char FAR *));
#endif	/*ftutyp_h*/

#ifdef	ftxtyp_h
#define	ftxseek(x,o)	ftxhsk(ftapih,x,o)
extern int FTAPIFUN	ftxhsk FTPL((FTAPIH, FTXH, FTXOFF FAR *));
#define	ftxtell(x,o)	ftxhtl(ftapih,x,o)
extern int FTAPIFUN	ftxhtl FTPL((FTAPIH, FTXH, FTXOFF FAR *));
#endif	/*ftxtyp_h*/

#ifdef	ftfb_h
	/* prototypes which require ftfb.h and other headers */
#include "ftfgfunc.h"
#endif
#ifdef	FTCPP
}
#endif
