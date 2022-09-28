/*static char *sccsid="@(#) fthandle.h (1.6)  14:21:06 07/08/97";

   fthandle.h -- typedefs for handles and other simple objects

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------
  | swr 5.1B 91/12/10 | 91101008: change ftapsize decl from PASCAL to FTAPIFUN
  | MM  5.1B 91/10/08 | declare ftfuqini(), ftgminit()
  | hjb 5.1B 91/10/04 | WIN: ftugenvw()
  | hjb 5.1B 91/10/01 | ftucnstr() source parameter changed to FAR
  | StM 5.1B 91/09/26 | 91080501: hpux ANSI ftu[r]indx parm: (const char *, int)
  | hjb 5.1B 91/09/05 | ftucvstr definition depends on FTHSDM
  | hjb 5.1B 91/09/05 | ftucnstr(), ftucvstr()
  | cps	5.1B 91/09/04 | Changes for Windows VSS (ftqhstrt prototype)
  | DG	5.1B 91/09/03 | move ftufulfn prototype to ftgfuncs.h from here
  | swr 5.1B 91/08/27 | 91011401: Mac: def'd FTGMH
  | swr 5.1B 91/08/27 | 91011401: Mac: added proto for ftumnrlc(); def'd FTAPSH
  | swr 5.1B 91/08/27 | ftgmfcls now void
  | DG	5.1B 91/08/23 | ftpanic is PASCAL, not FTAPIFUN
  | ah  5.1B 91/08/22 | 91070504 move ftpanic from ftprotos.h
  | hjb 5.1B 91/07/23 | 91071805: ftgmess not in DLL (WIN); ftumopn, etc., is!
  | hjb 5.1B 91/06/24 | prototypes for ftbcfget, ftbcfput removed to ftprotos.h
  | PW  5.1B 91/06/21 | prototypes for ftbcfget, ftbcfput
  | swr 5.1B 91/05/27 | 91012901: added #ifdef FTCPP (C++ safe links)
  | PW  5.1A 91/05/27 | change FTAPSH to FTHNDL in prototypes for ftumopn, ...
  | PW  5.1A 91/05/15 | ftumopn, ftumget, ftumcls, ftgmfcls
  | wtr 5.1A 91/05/14 | don't define ftuindx & fturindx in sun environment
  | hjb 5.1A 91/04/30 | ftucfstr() second parameter is CONST pointer
  | bf  5.1A 91/04/15 | FTBICALL ftbistrt
  | bf  5.0E 91/04/11 | FTHANSIC: declare ftuprntf,ftusprnt with ...
  | bf  5.1A 91/04/02 | ftqhstrt: add FTSH parameter; DOS: define ftuint21
  | bf  5.1A 91/04/02 | ftbxix,ftutfnam: FAR pointers
  | hjb 5.1W 91/03/22 | ftbistrt,ftewinst,ftpwinst,ftxwinst not in DLL
  | hjb 5.1W 91/03/21 | add ftufprnf; ftbufdmp() prototype change
  | bf  5.0W 91/03/06 | add ftuttd
  | bf  5.0W 91/03/05 | FTAPIH is NEAR only for Windows (not DOS or OS/2)
  | cps 5.1A 91/02/27 | Add ftqh* function prototypes
  | hjb 5.1W 91/02/22 | 90013001: FTAPIH is NEAR (for LDM); ftuargs() prototype
  | bf  5.0W 91/02/04 | drop FTDLFAR
  | hjb	5.0W 91/01/31 | ftfxhini(),ftfxhopn(),ftfxhtrm(),ftfxhnit() need FTAPIH
  | hjb	5.0W 91/01/28 | ftpigph() takes FTAPIH
  | bf  5.0W 91/01/22 | ftdbg,ftfdbg
  | hjb	5.0W 91/01/18 | ftnpeek(),ftnprntf(),ftnscanf() need ftapih
  | hjb	5.0W 91/01/10 | DLL repackaging: ftntstr,ftuspawn,ftuffind,ftuflxpnd
  | hjb	5.0W 91/01/09 | FTAPIFUN ftewinst,ftpwinst
  | bf  5.0W 91/01/08 | FTEXPORT ftumnrlc [hjb adds: PASCAL]
  | hjb	5.0W 91/01/03 | FTAPIFUN ftmfropn(),etc. required by ftpit
  | hjb	5.0W 91/01/02 | add prototype for ftnpeek(); change ftuprntf()
  | bf  5.0W 90/12/31 | ftscreate -> ftscreat [fixed 91/01/02 by hjb]
  | hjb	5.0W 90/12/31 | function prototype for ftbhen()'s callback function
  | bf  5.0W 90/12/20 | FTDLFAR -> FAR; [hjb:] add ftfxsf(), drop ftfxsxit()
  | bf  5.0W 90/12/12 | move FTGMH from ftgmem.h; add FTFH; fix ftmfxrio
  | hjb	5.0W 90/12/10 | ftfxhini(),ftfxhopn() prototype changes
  | hjb 5.0W 90/12/05 | DLL repackaging; move ftutw() to ftutypes.h
  | bf	5.0W 90/11/16 | change ftuprntf etc. to CDECL
  | bf	5.0W 90/11/07 | add FTLOADDS PASCAL to ftuclist prototype
  | bf	5.0W 90/11/06 | move signal handling functions to ftsignal.h
  | DG	5.0D 90/08/15 | ftnfhloc: node, nfid parms -> (char FTDLFAR **)
  | DAT 5.0D 90/08/11 | SM: VM port
  | bf	5.0D 90/08/08 | define FTNFH unconditionally
  | bf	5.0D 90/08/07 | add FTAPIH parameter to ftnconn,ftnucon,ftnfhloc
  | swr 5.0D 90/07/27 | 90071001: added protos for ftcbfsiz(), ftcsetbf()
  | DG	5.0D 90/07/19 | test ftwind_h for ftuw?clp prototypes
  | ah	5.0D 90/06/28 | remove FTBIGCAT #ifdefs
  | rtk 5.0C 90/06/07 | avoid nested comments
  | swr 5.0C 90/05/31 | ftfusgr: first arg is unsigned[]
  | bf	5.0C 90/05/15 | FTHNULL
  | DG	5.0C 90/05/01 | ftubrk() ftufix() prototypes from fthio.h
  | DG	5.0C 90/04/27 | drop ftscopen()
  | DG	5.0C 90/04/12 | add ftshropn(); drop ftswinst()
  | bf	5.0C 90/03/28 | fix ftnconn,ftnucon prototypes
  | PW	5.0C 90/03/26 | add prototypes for ftuindx(), fturindx()
  | DG	5.0C 90/03/19 | add ftmfsutp
  | bf	5.0B 90/03/14 | add ftuclist prototype
  | bf	5.0B 90/03/02 | add ftbufdmp prototype (90030205)
  | DG	5.0B 90/02/28 | ftuw?clp
  | cs	5.0B 90/02/28 | prototype change for ftmfocrt(), ftmfoget()
  | DG	5.0B 90/02/21 | FTNOD
  | DG	5.0B 90/02/12 | ftapsize()
  | bf	5.0B 90/01/11 | change ftususer return value from void to int
  | DG	5.0A 90/01/10 | add ftmfxrio prototype
  | cs	5.0B 90/01/09 | add ftmfcobj() prototype
  | bf	5.0B 90/01/05 | FTREG
  | DG	5.0A 89/12/20 | change FTAPSH, various pool manager functions
  | bf	5.0A 89/12/15 | don't declare ftusleep if it's a macro
  | bf	5.0A 89/12/13 | ftusleep now takes a long, returns void
  | bf	5.0A 89/12/08 | ftubreak,ftustrap,ftusrest if not FTHNOSIG
  | bf	5.0  89/11/29 | revise ftshstrt; move FTAPSH,FTAPSP from ftsi.h
  | bf	5.0  89/11/16 | revise ftbistrt
  | bf	5.0  89/11/16 | typedef FTAH FTBH
  | ah	5.0  89/11/15 | Changed typedef for FTAH to USHORT
  | KRL 5.0  89/11/11 | ftfr...() from fthio.h
  | rtk 5.0  89/11/09 | add ftppseqp()
  | bf	5.0  89/11/06 | Created.
  |-------------------|---------------------------------

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.	  */

#ifndef fthndl_h
#define fthndl_h
#ifndef fthio_h
#include "fthio.h"
#endif
#ifdef FTHVMCMS
#include <types.h>
#endif

	/* scalar types */
typedef long	FTCID;
typedef long	FTVCC;
#ifndef FTHVMCMS
typedef long	FTTIME;
#else
typedef time_t	FTTIME;
#endif
typedef USHORT	FTFID;
typedef char 	FTQDATA;

	/* handles */
typedef USHORT	FTAH;
typedef FTAH	FTBH;
typedef FTHNDL	FTCH;
typedef FTHNDL	FTCMH;
typedef FTHNDL	FTIH;
typedef FTHNDL	FTMFFH;		/* instance handle of FTMFFB */
typedef short	FTMFOH;		/* object handle, index into index block */
typedef int	FTMFRH;		/* handle for FTMFD structure */
typedef FTHNDL	FTNFH;
typedef FTHNDL	FTSH;
typedef FTHNDL	FTXH;
typedef FTHNDL	FTQH;

	/* type of handle set by ftgmbrak, ftumfbrk */
#ifdef	FTHMSDOS
#ifdef	FTHWIN
typedef	USHORT		FTGMH;
#else
	/* unsigned for handle/segment/selector	*/
typedef	unsigned	FTGMH;
#endif
#else	/*!FTHMSDOS*/
#ifdef	mac		/*91011401*/
#define FTGMH		FTHNDL
#else	/*!mac*/
typedef char *		FTGMH;
#endif	/*!mac*/
#endif	/*!FTHMSDOS*/

typedef FTGMH		FTFH;

#define FTHNULL ((FTHNDL)0)

	/* generic structure type */
typedef struct	_FTSTRUC	{
	int	integer;
}	FTSTRUC;

	/* typedef for near/far pointers used as function return values */
#ifdef	FTHMSDOS
typedef char NEAR *	FTNP;
typedef char FAR *	FTFP;
#else	/*!FTHMSDOS*/
#ifdef	M_XENIX
typedef char NEAR *	FTNP;
typedef char FAR *	FTFP;
#else	/*!M_XENIX*/
typedef char *	FTNP;
typedef char *	FTFP;
#endif	/*!M_XENIX*/
#endif	/*!FTHMSDOS*/

	/* private typedefs used for API functions */
typedef FTSTRUC *FTPDP;
#ifdef	mac		/*91011401*/
typedef	FTHNDL	FTAPSH;			/* private structure handle */
#else
typedef	USHORT	FTAPSH;			/* private structure handle */
#endif
typedef FTNP	FTAPSP;			/* private structure pointer */

	/* API instance handle */
#ifdef	FTHWIN
typedef FTSTRUC NEAR *	FTAPIH;
#else
typedef FTSTRUC *	FTAPIH;
#endif

#ifndef FTNOD
	/* provide external reference for global API handle declared in
	   ftpglob.h (for ftapiini()) */
extern	FTAPIH	ftgapih;	/* reserved global API handle */
	/* ftapih is undef'd in ftapi.h */
#define ftapih	ftgapih
#endif	/*!FTNOD*/

	/* cause register declarations to go away if debugging */
#ifdef	FTDEBUG
#define FTREG
#else
#define FTREG	register
#endif

	/* "FTAPIFUN except in Windows" */
#ifdef	FTHWIN
#define FTAPIFXW PASCAL
#else
#define FTAPIFXW FTAPIFUN
#endif

	/* FUNCTION PROTOTYPES */
/* WHERE POSSIBLE, PLEASE MAINTAIN IN ALPHABETICAL ORDER BY FUNCTION NAME */
	/* FTPL means prototype used only if FTLINT is defined */
	/* the definition for FTPL is in fthio.h */

#ifdef	FTCPP
extern	"C" {
#endif
	/* fta... */
extern int FTAPIFUN	ftapiini FTPL((char FAR * FAR *, int,
			    long FAR *, int, FTAPIH FAR *));
extern int FTAPIFUN	ftapitrm FTPL((FTAPIH));
extern int FTAPIFUN	ftapsize FTPL((FTAPIH, unsigned FAR *,
			    unsigned FAR *));

	/* ftb... */
#define ftbclose(b)			ftbhcl(ftapih,b)
extern int FTAPIFUN	ftbhcl FTPL((FTAPIH, FTBH));
extern int FTAPIFXW	ftbistrt FTPL((FTAPIH, char FAR *, char FAR *, long,
			    long, long, long, FTIH FAR *));
#define ftblock(f,m)			ftbhlk(ftapih,f,m)
extern int FTAPIFUN	ftbhlk FTPL((FTAPIH, char FAR *, int));
#define ftbset(b,s,a)			ftbhsi(ftapih,b,s,a)
extern int FTAPIFUN	ftbhsi FTPL((FTAPIH, FTBH, int, char FAR *));
#define ftbskill(b)			ftshte(ftapih,b)
extern int FTAPIFUN	ftshte FTPL((FTAPIH, FTBH));
#define ftbstat(b,s,a)			ftbhgi(ftapih,b,s,a)
extern int FTAPIFUN	ftbhgi FTPL((FTAPIH, FTBH, int, char FAR *));
#define ftbstrt(b)			ftshin(ftapih,b)
extern int FTAPIFUN	ftshin FTPL((FTAPIH, FTBH));
extern void PASCAL	ftbufdmp FTPL((FTAPIH, FILE *, char FAR *, int));
#define ftbxopen(n,c,i)			ftbxop(ftapih,n,c,i)
extern FTBH FTAPIFUN	ftbxop FTPL((FTAPIH, char FAR *, char FAR *,
			    char FAR *));
#define ftbxuex(n,l,m,s,t)		ftbxix(ftapih,n,l,m,s,t)
extern int FTAPIFUN	ftbxix FTPL((FTAPIH, char FAR *, char FAR *, char FAR *,
			    long, long));

	/* ftc... */
#define ftcbfsiz(c)			ftchgi(ftapih,c)
extern unsigned int FTAPIFUN ftchgi FTPL((FTAPIH, FTCH));
#define ftcclose(c)			ftchcl(ftapih,c)
extern int FTAPIFUN	ftchcl FTPL((FTAPIH, FTCH));
extern int FTAPIFUN	ftcdel FTPL((FTAPIH, FTCH, FTCID));
extern int FTAPIFUN	ftcfget FTPL((FTAPIH, FTCH, FTFID FAR *, FTVCC FAR *,
			    USHORT FAR *));
extern int FTAPIFUN	ftcfput FTPL((FTAPIH, FTCH, FTFID, short));
#define ftcfulfn(r,b,f)			ftufma(ftapih,r,b,f)
extern int FTAPIFUN	ftufma FTPL((FTAPIH, char FAR *, FTBH, char FAR *));
#define ftcmclse(c)			ftcmcl(ftapih,c)
extern int FTAPIFUN	ftcmcl FTPL((FTAPIH, FTCMH));
#define ftcopen(b,n,f,m)		ftchop(ftapih,b,n,f,m)
extern FTCH FTAPIFUN	ftchop FTPL((FTAPIH, FTBH, int, int, int));
#define ftcreopn(b,c,n,f,m)		ftchro(ftapih,b,c,n,f,m)
extern FTCH FTAPIFUN	ftchro FTPL((FTAPIH, FTBH, FTCH, int, int, int));
#define ftcrlse(c)			ftchun(ftapih,c)
extern int FTAPIFUN	ftchun FTPL((FTAPIH, FTCH));
#define ftcseek(c,i)			ftchsk(ftapih,c,i)
extern int FTAPIFUN	ftchsk FTPL((FTAPIH, FTCH, FTCID));
#define ftcsetbf(c,b)			ftchsi(ftapih,c,b)
extern int FTAPIFUN	ftchsi FTPL((FTAPIH, FTCH, char FAR *));
extern int FTAPIFUN	ftcsfree FTPL((FTAPIH, FTCH));
#define ftcsput(c,s)			ftcrsi(ftapih,c,s)
extern int FTAPIFUN	ftcrsi FTPL((FTAPIH, FTCH, int));

	/* ftd... */
#ifdef	FTDLENV
extern void PASCAL	ftdllini FTPL((FTAPIH, unsigned, unsigned));
#endif

	/* ftf... */
#define ftfcef()			ftcfte(ftapih)
extern int FTAPIFUN	ftcfte FTPL((FTAPIH));
#define ftfcpc(c)			ftcfpf(ftapih,c)
extern int FTAPIFUN	ftcfpf FTPL((FTAPIH, int));
#define ftfcsf(f)			ftcfin(ftapih,f)
extern int FTAPIFUN	ftcfin FTPL((FTAPIH, FTFID));
#define ftfcupd()			ftcfgi(ftapih)
extern int FTAPIFUN	ftcfgi FTPL((FTAPIH));
extern int FTAPIFUN	ftffmtl FTPL((long, unsigned, int, char FAR *));
extern int FTAPIFUN	ftfmtl FTPL((long, unsigned, int, char FAR *));
	/* ftfr... */
extern long PASCAL	ftfrseek FTPL((int, long, int));
	/* ftfu... */
extern void FTAPIFUN	ftfucrnd FTPL((int FAR *, int FAR *, int));
extern int FTAPIFUN	ftfuernd FTPL((int, int FAR *, int));
extern int FTAPIFUN	ftfufrnd FTPL((int FAR *, int FAR *, int, char FAR *,
			    int));
extern void FTAPIFUN	ftfuqini FTPL((void));
#define	ftfuqfmt(d,v,n,k,b,l,s)		ftfuqf(ftapih,d,v,n,k,b,l,s)
extern int FTAPIFUN	ftfuqf FTPL((FTAPIH, char FAR *, int FAR *, int, USHORT,
			    char FAR *, int, int));
extern int FTAPIFUN	ftfuqrnd FTPL((int, int FAR *, int));
extern void FTAPIFUN	ftfuqspc FTPL((char FAR *));
extern int FTAPIFUN	ftfusgr FTPL((unsigned FAR *, int, int FAR *, int,
			    int, int FAR *));
	/* ftfx... */
#define ftfxclse(f)			ftfxcl(ftapih,f)
extern int FTAPIFUN	ftfxcl FTPL((FTAPIH, int));
#define ftfxfopn(f,p,t,e,z,c)		ftfxfo(ftapih,f,p,t,e,z,c)
extern int FTAPIFUN	ftfxfo FTPL((FTAPIH, int, char FAR *, char FAR *, int,
			    int, int));
extern int FTAPIFUN	ftfxhini FTPL((FTAPIH, int, char FAR *, unsigned,
			    FTAPSH FAR *));
extern int FTAPIFUN	ftfxhtrm FTPL((FTAPIH, FTAPSH));
extern int FTAPIFUN	ftfxhopn FTPL((FTAPIH, FTAPSH, char FAR *,
			    unsigned FAR *, int, int, int, unsigned));
extern USHORT FTAPIFUN	ftfxhnit FTPL((FTAPIH, FTAPSH, unsigned));
#define ftfxinit()			ftfxin(ftapih)
extern int FTAPIFUN	ftfxin FTPL((FTAPIH));
#define ftfxlget(x,b,l,s,i,v)		ftfxrd(ftapih,x,b,l,s,i,v)
extern int FTAPIFUN	ftfxrd FTPL((FTAPIH, int, char FAR *, int, int FAR *,
			    int FAR *, int));
extern int FTAPIFUN	ftfxsf FTPL((FTAPIH, char FAR * (PASCAL *)(),
			    char FAR * (PASCAL *)()));
#define ftfxsmrg(f,d)			ftfxsm(ftapih,f,d)
extern int FTAPIFUN	ftfxsm FTPL((FTAPIH, int, char FAR *));
#define ftfxterm(f)			ftfxte(ftapih,f)
extern int FTAPIFUN	ftfxte FTPL((FTAPIH, int));
#define ftfxthaw()			ftfxun(ftapih)
extern int FTAPIFUN	ftfxun FTPL((FTAPIH));
#define ftfxvpos(f,o,w,u)		ftfxsk(ftapih,f,o,w,u)
extern long FTAPIFUN	ftfxsk FTPL((FTAPIH, int, long, int, int));

	/* ftg... */
#define ftgmess(n)			ftutgm(ftapih,n)
extern char FAR * FTAPIFXW ftutgm FTPL((FTAPIH, unsigned));
#define ftgnmess(n)			ftutgn(ftapih,n)
extern char FAR * FTAPIFXW ftutgn FTPL((FTAPIH, unsigned));
#define ftgmerno(e)			ftutge(ftapih,e)
extern char FAR * FTAPIFXW ftutge FTPL((FTAPIH, int));
extern void FTAPIFXW 	   ftgminit FTPL((void));

	/* ftmf... */
extern FTMFFH FTAPIFUN	ftmfinit FTPL((FTAPIH, char FAR *, int, char FAR *));
extern int PASCAL	ftmfcobj FTPL((FTMFFH,FTMFRH,FTMFRH));
extern FTMFOH FTAPIFUN	ftmfocrt FTPL((FTMFFH, unsigned, unsigned, unsigned, 
			    unsigned, char FAR *));
extern int FTAPIFUN	ftmfodel FTPL((FTMFFH, FTMFOH));
extern FTMFOH FTAPIFUN	ftmfoget FTPL((FTMFFH, FTMFOH, unsigned, unsigned,
			    int));
extern FTTIME FTAPIFUN	ftmfomod FTPL((FTMFFH, unsigned, int));
extern FTMFOH FTAPIFUN	ftmfosch FTPL((FTMFFH, char FAR *, unsigned));
extern int FTAPIFUN	ftmfrcls FTPL((FTMFFH, FTMFRH));
extern int FTAPIFUN	ftmfrget FTPL((FTMFFH,FTMFRH,int,long,char FAR *,int));
extern FTMFRH FTAPIFUN	ftmfropn FTPL((FTMFFH, FTMFOH, unsigned));
extern int FTAPIFUN	ftmfrput FTPL((FTMFFH,FTMFRH,int,long,char FAR *,int));
extern unsigned long PASCAL ftmfrtel FTPL((FTMFFH, FTMFRH));
extern int PASCAL	ftmfrsek FTPL((FTMFFH, FTMFRH, unsigned long));
extern int PASCAL	ftmfsutp FTPL((FTMFFH, FTMFRH, unsigned));
extern int FTAPIFUN	ftmfterm FTPL((FTMFFH));
extern int PASCAL	ftmfxrio FTPL((FTAPIH, FTMFFH, unsigned, char FAR *,
			    short FAR *, unsigned));

	/* ftn... */
#ifdef	FTHNET
extern FTNFH FTAPIFUN	ftnconn FTPL((FTAPIH, char FAR *, char FAR *));
extern int		ftnfhloc FTPL((FTAPIH, FTNFH, char **, char **));
extern int		ftnterm FTPL((FTAPIH));
extern int FTAPIFUN	ftnucon FTPL((FTAPIH, FTNFH, int));
#endif	/*FTHNET*/
extern void PASCAL	ftnpath FTPL((char FAR *));
extern int FTAPIFUN	ftnpeek FTPL((FTAPIH, FTNFH));
#define	ftntstr(i,l,o,s,m,n)		ftnttr(ftapih,i,l,o,s,m,n)
extern int FTAPIFUN	ftnttr FTPL((FTAPIH, const char FAR *, int, char FAR *,
			    int, int, int));

	/* ftp... */
extern void PASCAL	ftpanic FTPL((FTAPIH, FILE *, char FAR *, char FAR *));
#ifndef ftpfpstk
extern	int		ftpfflay FTPL((int, int));
extern	int		ftpffstk FTPL((int, int));
extern	FTFP		ftpfplay FTPL((int, int, unsigned	*));
extern	FTFP		ftpfpstk FTPL((int, int, unsigned	*));
extern	int		ftpftlay FTPL((int, FTFP));
extern	int		ftpftstk FTPL((int, FTFP));
#endif
extern int		ftpiinit FTPL((unsigned, unsigned));
extern FTHNDL FTAPIFUN	ftpigph FTPL((FTAPIH));
extern int		ftpiterm FTPL((void));
	/* ftpm... */
extern int		ftpmalay FTPL((int, unsigned));
extern int FTEXPORT PASCAL ftpmastk FTPL((FTHNDL, unsigned, FTAPSH FAR *));
extern int		ftpmclay FTPL((int));
extern int FTEXPORT PASCAL ftpmcstk FTPL((FTHNDL));
extern int		ftpmhlay FTPL((int));
extern FTAPSH FTEXPORT PASCAL ftpmhstk FTPL((FTHNDL));
extern FTHNDL FTEXPORT PASCAL ftpminit FTPL((FTAPIH, unsigned, unsigned, unsigned,
				unsigned));
extern int		ftpmnlay FTPL((int));
extern int FTEXPORT PASCAL ftpmnstk FTPL((FTHNDL));
extern int FTEXPORT PASCAL ftpmrstk FTPL((FTHNDL, unsigned, FTNP FAR *));
extern int		ftpmslay FTPL((int));
extern unsigned FTEXPORT PASCAL ftpmsstk FTPL((FTHNDL, unsigned FAR *));
extern int PASCAL	ftpmstat FTPL((FTHNDL, FTWHO));
extern int FTEXPORT PASCAL ftpmterm FTPL((FTHNDL));
extern unsigned		ftpmxlay FTPL((int));
extern unsigned FTEXPORT PASCAL ftpmxstk FTPL((FTHNDL));
	/* ftpn... */
extern	int		ftpnflay FTPL((int, int));
extern	int FTEXPORT PASCAL ftpnfstk FTPL((FTHNDL, FTAPSH));
extern	FTNP		ftpnplay FTPL((int, int, unsigned *));
extern	FTNP FTEXPORT PASCAL ftpnpstk FTPL((FTHNDL, FTAPSH, unsigned FAR *));
extern int		ftpntlay FTPL((int, FTNP));
extern int FTEXPORT PASCAL ftpntstk FTPL((FTHNDL, FTNP));

	/* ftpp... */
extern int FTAPIFUN	ftppallp FTPL((FTAPIH, int, unsigned, int, FTAH,
			    FTPDP FAR *));
extern int FTAPIFUN	ftppfrep FTPL((FTAPIH, FTPDP, int));
extern int FTAPIFUN	ftpphtop FTPL((FTAPIH, FTAH, FTPDP FAR *));
extern int FTAPIFUN	ftppptoh FTPL((FTAPIH, FTPDP, FTAH FAR *));
extern int FTAPIFUN	ftppseqp FTPD((FTAPIH, int, FTAH, FTPDP FAR *));

	/* ftr... */
#define ftfrcls(rfd)	ftrfcls(ftapih,rfd)
extern int FTAPIFUN	ftrfcls FTPL((FTAPIH,int));
extern int FTAPIFUN	ftrfdel FTPL((FTAPIH, char FAR *));
extern int PASCAL	ftrfinit FTPL((FTAPIH));
#define ftfrio(rfd,offset,mode,buf,len) ftrfio(ftapih,rfd,offset,mode,(buf),len)
extern int FTAPIFUN	ftrfio FTPL((FTAPIH, int, long, unsigned, char FAR *,
			    unsigned));
#define ftfropen(path,oflag,param)	ftrfopen(ftapih,(path),(oflag),(param))
extern int FTAPIFUN	ftrfopen FTPL((FTAPIH, char FAR *, int, char FAR *));
extern int FTAPIFUN	ftrflock FTPL((FTAPIH, int, int, long));
#define ftfrsize(rfd)	ftrfsize(ftapih,rfd)
extern long FTAPIFUN	ftrfsize FTPL((FTAPIH, int));
#define ftfrtell(rfd)	ftrftell(ftapih,rfd)
extern long FTAPIFUN	ftrftell FTPL((FTAPIH, int));
extern int PASCAL	ftrfterm FTPL((FTAPIH));
#define ftfrtofd(rfd)	ftrftofd(ftapih,rfd)
extern int FTAPIFUN	ftrftofd FTPL((FTAPIH, int));

	/* fts... */
#define ftsclose(s,f)			ftshcl(ftapih,s,f)
extern int FTAPIFUN	ftshcl FTPL((FTAPIH, FTSH, int));
#define ftscreat(b,n,f,s,t,l,g)	ftshcr(ftapih,b,n,f,s,t,l,g)
extern FTSH FTAPIFUN	ftshcr FTPL((FTAPIH, FTBH FAR *, unsigned, unsigned,
			    char FAR * FAR *, int, long FAR *, int));
#define ftsgcnt(s)			ftshgc(ftapih,s)
extern int FTAPIFUN	ftshgc FTPL((FTAPIH, FTSH));
#define ftsgetc(s,r,d)			ftshgf(ftapih,s,r,d)
extern FTCID FTAPIFUN	ftshgf FTPL((FTAPIH, FTSH, FTCID, int FAR *));
#define ftsgev(s,v)			ftshrd(ftapih,s,v)
extern FTVCC FTAPIFUN	ftshrd FTPL((FTAPIH, FTSH, FTVCC FAR *));
#define ftsgkey(s,k)			ftshrk(ftapih,s,k)
extern int FTAPIFUN	ftshrk FTPL((FTAPIH, FTSH, long FAR *));
#define ftsgwght(s,w)			ftshgw(ftapih,s,w)
extern int FTAPIFUN	ftshgw FTPL((FTAPIH, FTSH, long FAR *));
extern int FTAPIFUN	ftshropn FTPL((FTAPIH, FTBH FAR *, char FAR *,
			    FTSH FAR *));
extern int FTAPIFUN	ftshstrt FTPL((FTAPIH, FTBH FAR *, unsigned, FTCID,
			    unsigned, const char FAR *, char FAR *, long,
			    FTSH FAR *));
extern int PASCAL	ftsmclse FTPL((FTAPIH));
extern int FTAPIFUN	ftsmopen FTPL((FTAPIH, char FAR *, unsigned));
#define ftsopen(b,n,x,i,c,d,f,q,r)	ftshop(ftapih,b,n,x,i,c,d,f,q,r)
extern FTSH FTAPIFUN	ftshop FTPL((FTAPIH, FTBH FAR *, unsigned, FTCID,
			    unsigned,
			    int (PASCAL FAR *) FTPL((FTSH, char FAR *)),
			    char FAR *, unsigned, char FAR *, char FAR *));
#define ftsputc(s,d,n,c)		ftshpf(ftapih,s,d,n,c)
extern FTCID FTAPIFUN	ftshpf FTPL((FTAPIH, FTSH, FTCID FAR *, FTCID, int));
#define ftsrclse(r)			ftsxcl(ftapih,r)
extern int FTAPIFUN	ftsxcl FTPL((FTAPIH, char FAR *));
#define ftsrlink(s,r)			ftsxcr(ftapih,s,r)
extern char FAR * FTAPIFUN ftsxcr FTPL((FTAPIH, FTSH, char FAR *));
#define ftsropen(r)			ftsxop(ftapih,r)
extern FTSH FTAPIFUN	ftsxop FTPL((FTAPIH, char FAR *));
#define ftsrstat(r,s,a)			ftsxgi(ftapih,r,s,a)
extern int FTAPIFUN	ftsxgi FTPL((FTAPIH, char FAR *, int, char FAR *));
#define ftsstat(s,i,a)			ftshgi(ftapih,s,i,a)
extern int FTAPIFUN	ftshgi FTPL((FTAPIH, FTSH, int, char FAR *));
#define ftswrix(s,k,n)			ftshwr(ftapih,s,k,n)
extern int FTAPIFUN	ftshwr FTPL((FTAPIH, FTSH, long FAR *, FTCID));

	/* ftu... */
extern void	ftuargs FTPL((FTAPIH, int, char **, char *, char **, char **,
			    char **));
extern int FTAPIFUN	ftumcls FTPL((FTAPIH, FTHNDL));
extern int FTAPIFUN	ftumget FTPL((FTAPIH, FTHNDL, int, char FAR *, int)); 
extern void FTAPIFUN	ftgmfcls FTPL((FTAPIH));
extern int FTAPIFUN 	ftumopn FTPL((FTAPIH, char FAR *, char FAR *,
			    FTHNDL FAR *));
#ifdef	FTHDEVEL
extern int	ftubrk FTPL((char *, char *, char *, FTHNDL *));
extern char	*ftufix FTPL((char *, char *, FTHNDL));
#endif
#ifdef	FTHMSDOS
	/* these functions are only compiled in MS-DOS */
extern char FAR * PASCAL ftucfstr(char FAR *, CONST char FAR *);
extern char NEAR * PASCAL ftucnstr(char NEAR *, CONST char FAR *);
#ifdef	FTHSDM
#define	ftucvstr	ftucnstr
#else	/*!FTHSDM*/
#define	ftucvstr	ftucfstr
#endif	/*!FTHSDM*/
extern int PASCAL	ftugcd(int, char FAR *);
extern int PASCAL	ftugdfs(int, unsigned long FAR *);
extern int PASCAL	ftugdsk(void);
#ifdef	FTHWIN
extern char FAR * FTAPIFUN ftugenvw FTPL((const char FAR *));
extern int PASCAL	ftuint21(void FAR *, void FAR *, void FAR *);
#else
#define ftuint21	intdosx
#endif
extern int PASCAL	ftulfstr(char FAR *);
#else	/*!FTHMSDOS*/
#define	ftucfstr	ftucstr
#define	ftucnstr	ftucstr
#define	ftucvstr	ftucstr
#endif	/*!FTHMSDOS*/
extern int FTAPIFUN	ftuci FTPL((char FAR *, int));
extern long FTAPIFUN	ftucl FTPL((char FAR *, int));
#define ftuclist(f,d,l)			ftbhen(ftapih,f,d,l)
extern int FTAPIFUN	ftbhen FTPL((FTAPIH, int (FTLOADDS PASCAL FAR *)
    FTPL((char FAR *, char FAR *, int, char FAR *, char FAR *)),
			    char FAR *, unsigned));
extern char FAR * FTAPIFUN ftucstr FTPL((char FAR *, CONST char FAR *));
#ifndef sun
#ifndef FTHMSDOS
#ifndef __hpux
extern char *		ftuindx FTPL((char *, char));
extern char *		fturindx FTPL((char *, char));
#else
#ifdef __STDC__		/* HP9000/800: cc -Aa */
extern char *		ftuindx FTPL((const char *, int));
extern char *		fturindx FTPL((const char *, int));
#else
extern char *		ftuindx FTPL((char *, char));
extern char *		fturindx FTPL((char *, char));
#endif /*!__STDC__*/
#endif /*__hpux*/
#endif /*!FTHMSDOS*/
#endif /*!sun*/
extern void FTAPIFUN	ftuddflt FTPL((char FAR *));
extern FTTIME FTAPIFUN	ftudnix FTPL((FTTIME, int));
extern int FTAPIFUN	ftudnjss FTPL((FTTIME, int, char FAR *, int));
extern int FTAPIFUN	ftudntos FTPL((FTTIME, int, char FAR *));
extern FTTIME FTAPIFUN	ftudston FTPL((char FAR *, int));
#define ftufcopy(from,to,flags)		ftufcp(ftapih,from,to,flags)
extern int FTAPIFUN	ftufcp FTPL((FTAPIH, char FAR *, char FAR *, int));
#define	ftuffind(f,p,n)			ftutfi(ftapih,f,p,n)
extern int PASCAL	ftutfi FTPL((FTAPIH, char FAR *, char FAR *,
			    char FAR *));
#define ftufgval(k,f)			ftfgrk(ftapih,k,f)
extern char FAR * FTAPIFUN ftfgrk FTPL((FTAPIH, char FAR *, char FAR *));
extern char *		ftuflfnd FTPL((char *, char *));
#define	ftuflxpnd(f,n,w,s)		ftutxf(ftapih,f,n,w,s)
extern char FAR * FTAPIFUN ftutxf FTPL((FTAPIH, char FAR *, char FAR *,
			    char FAR *, int));
#define ftufmove(from,to,flags)		ftufmv(ftapih,from,to,flags)
extern int FTAPIFUN	ftufmv FTPL((FTAPIH, char FAR *, char FAR *, int));
#define ftufnorm(f)			ftufnr(ftapih,f)
extern int FTAPIFUN	ftufnr FTPL((FTAPIH, char FAR *));
extern int FTAPIFUN	ftufscnl FTPL((char FAR *, char FAR *, long FAR *));
extern char *		ftufullp FTPL((int));
extern int FTAPIFUN	ftugwd FTPL((char FAR *, int));
extern int FTAPIFUN	ftuic FTPL((int, char FAR *, int));
extern int FTAPIFUN	ftulc FTPL((long, char FAR *, int));
extern int FTAPIFUN	ftumcmp FTPL((char FAR *, char FAR *, int));
extern int FTAPIFUN	ftumkdir FTPL((FTAPIH, char FAR *));
#ifdef	FTHWIN
extern char NEAR * FTEXPORT PASCAL ftumnrlc(FTAPIH, char NEAR *, unsigned);
#endif
#ifdef	mac		/*91011401*/
extern char *		ftumnrlc(FTAPIH, char NEAR *, unsigned);
#endif
extern int PASCAL	ftuncmp FTPL((UCHAR FAR *, UCHAR FAR *, int));
#ifdef	FTHANSIC
extern int CDECL	ftdbg(char *, ...);
#ifdef	FTHDOSSDM
extern int CDECL	ftfdbg(char *, ...);
extern int CDECL	ftufprtf(FILE *, char FAR *, ...);
#endif
extern int CDECL FTLOADDS FTEXPORT
			ftnprntf(FTAPIH, FTNFH, char FAR *, ...);
extern int CDECL FTLOADDS FTEXPORT
			ftnscanf(FTAPIH, FTNFH, char FAR *, ...);
extern int CDECL	ftufprnf(FTAPIH, FILE *, char FAR *, ...);
extern int CDECL	ftuprntf(FILE *, char *, ...);
extern int CDECL FTLOADDS FTEXPORT
			ftusprnt(FTAPIH, char FAR *, unsigned, char FAR *, ...);
#else	/*!FTHANSIC*/
#ifndef vms
	/* KLUGE -- variable number of parameters doesn't work in vms */
/*VARARGS1*/
extern int CDECL	ftdbg FTPL((char *, ...));
#ifdef	FTHDOSSDM
extern int CDECL	ftfdbg FTPL((char *, ...));
#endif
/*VARARGS2*/
extern int CDECL	ftuprntf FTPL((FILE *, char *, ...));
/*VARARGS3*/
extern int CDECL	ftufprnf FTPL((FTAPIH, FILE *, char FAR *, ...));
/*VARARGS4*/
extern int CDECL FTLOADDS FTEXPORT ftusprnt FTPL((FTAPIH, char FAR *, unsigned,
			    char FAR *, ...));
/*VARARGS3*/
extern int CDECL FTLOADDS FTEXPORT ftnprntf FTPL((FTAPIH, FTNFH, char FAR *,
			    ...));
/*VARARGS3*/
extern int CDECL FTLOADDS FTEXPORT ftnscanf FTPL((FTAPIH, FTNFH, char FAR *,
			    ...));
#endif	/*!vms*/
#endif	/*!FTHANSIC*/
extern int FTAPIFUN	ftuscmp FTPL((UCHAR FAR *, UCHAR FAR *));
extern int PASCAL	ftusdlst FTPL((FTAPIH));
extern int PASCAL	ftusdtmp FTPL((FTAPIH));
extern int FTAPIFUN	ftusfsch FTPL((FTAPIH, char FAR *, unsigned,
			    char FAR *));
extern int FTAPIFUN	ftusftmp FTPL((FTAPIH, char FAR *));
#ifndef ftusleep
extern void PASCAL	ftusleep FTPL((long));
#endif
extern int FTAPIFUN	ftusscnl FTPL((char FAR *, char FAR *, long FAR *));
extern int FTAPIFUN	ftusstr FTPL((char FAR *, char FAR *));
#define	ftuspawn(p,c,f,x,d)		ftutst(ftapih,p,c,f,x,d)
extern int FTAPIFUN	ftutst FTPL((FTAPIH, char FAR *, char FAR *, int,
			    int (*)(), char FAR *));
#define ftususer(u,p)			ftunsi(ftapih,u,p)
extern int FTAPIFUN	ftunsi FTPL((FTAPIH, char FAR *, char FAR *));
#define	ftutdnam(d)			ftutdn(ftapih,d)
extern char FAR * FTAPIFUN ftutdn FTPL((FTAPIH, char FAR *));
extern int PASCAL	ftutfnam FTPL((char FAR *, char FAR *));
extern long FTAPIFUN	ftutime FTPL((void));
extern int FTAPIFUN	ftuttd FTPL((FTAPIH, char FAR *));
#ifdef	ftwind_h
extern int FTAPIFUN	ftuwgclp FTPL((FTAPIH, HWND, HANDLE FAR *,
			    unsigned FAR *));
extern int FTAPIFUN	ftuwpclp FTPL((FTAPIH, HWND, HANDLE, unsigned));
extern int FTAPIFUN	ftuwxclp FTPL((FTAPIH, HWND, HWND));
#endif
#ifdef	FTHWIN
extern int PASCAL	ftuwslnh FTPL((USHORT));
#endif	/*FTHWIN*/

	/* ftx... */
#define ftxclose(x)			ftxhcl(ftapih,x)
extern int FTAPIFUN	ftxhcl FTPL((FTAPIH, FTXH));
#define ftxgetw(x,d,w,o)		ftxhrd(ftapih,x,d,w,o)
extern int FTAPIFUN	ftxhrd FTPL((FTAPIH, FTXH, unsigned, UCHAR FAR *,
			    long FAR *));
#define ftxgez(x,v,d,o)			ftxhgf(ftapih,x,v,d,o)
extern FTFID FTAPIFUN	ftxhgf FTPL((FTAPIH, FTXH, unsigned, long FAR *,
			    long FAR *));
#define ftxopen(b,x,p,z,l,c)		ftxhop(ftapih,b,x,p,z,l,c)
extern FTXH FTAPIFUN	ftxhop FTPL((FTAPIH, FTBH, FTXH, UCHAR FAR *,
			    UCHAR FAR *, unsigned, unsigned));
#ifdef	FTHWIN
extern int PASCAL	ftewinst FTPL((USHORT, USHORT));
extern int PASCAL	ftpwinst FTPL((USHORT, USHORT));
extern int PASCAL	ftxwinst FTPL((USHORT, USHORT));
#endif

	/* ftqh... */

extern int FTAPIFUN	ftqhstrt FTPL((FTAPIH, FTBH, FTSH, long, char FAR *, 
			    char FAR *, char FAR *, FTQH FAR *));
extern int FTAPIFUN 	ftqhresm FTPL((FTAPIH, FTQH, long, int, FTQDATA FAR *));
extern int FTAPIFUN 	ftqhdel FTPL((FTAPIH, char FAR *));
#ifdef	FTCPP
}
#endif
#endif	/*!fthndl_h*/
