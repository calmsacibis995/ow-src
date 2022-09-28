/*static char *sccsid="@(#) ftads.h (1.6)  14:21:02 07/08/97";

   ftads.h -- stubs for disabled API services

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------
  | StM 5.1B 91/12/23 | 91120303: change ftqtsyn() and ftqtsuf() caling seq.
  | MM  5.1B 91/10/08 | prototype ftgminit()
  | MM  5.1B 91/10/02 | prototype ftqthtrm() to return int
  | ded 6.0A 91/09/27 | 91082201: add stubs ftqthini() fthltfre() and ftqthtrm()
  | cps 5.1B 91/09/04 | Changes for Windows VSS (remote FTADSCC)
  | hjb 5.1B 91/07/23 | WIN: message file stubs restored (not in DLL)
  | ded 5.1B 91/06/28 | Added stub for fticmpmk() ifndef FTADSCUPD 
  | hjb 5.1B 91/06/24 | ftnconn(),ftnopen(),ftnfind() take FAR pointers
  | PW  5.1A 91/05/17 | rm ftgmfcls - (module moved from ftgmess to ftumopn)
  | wtr 5.1A 91/05/16 | use FTSTRUC instead of FTFIG if !ftctyp_h
  | PW  5.1A 91/05/07 | remove ftrmess, add ftgmfcls
  | hjb 5.1A 91/05/02 | FTADS{RANK,VRNT,THES,BKCM,MODF;MSG+,SLOC+}; test DLL
  | bf  5.0E 91/04/15 | FTHANSIC: declare ftnprntf, ftnscanf with ...
  | bf  5.1A 91/04/12 | ftutgm,ftutgn,ftutge
  | DG	5.1A 91/04/10 | WIN: ftbistrt,ftbiresm are PASCAL, not FTAPIFUN
  | bf  5.1A 91/04/02 | FAR FTFB, FTADSICC
  | ah  5.1A 91/03/18 | remove ftcdncid() stub
  | cps 5.1A 91/02/27 | FTADSQI
  | hjb	5.0W 91/02/06 | ftnread takes FAR buffer
  | bf  5.0W 91/02/04 | drop FTDLFAR
  | hjb	5.0W 91/02/01 | ftnopen,ftnpacksz,ftnread,ftnver require ftapih
  | hjb	5.0W 91/01/18 | ftnfind,ftnpeek,ftnprntf,ftnscanf require ftapih
  | bf  5.0W 91/01/17 | add ftsmopen, fix ftrmess return
  | hjb	5.0W 91/01/09 | FTEXPORT PASCAL ftnlunk()
  | hjb	5.0W 91/01/02 | FTAPIFUN ftnpeek()
  | bf  5.0W 90/12/20 | rename ftapih to apih in function parameters
  | bf  5.0W 90/12/19 | FTDLFAR -> FAR
  | bf  5.0W 90/12/13 | FTAPIFUN; rename catalog update functions
  | hjb	5.0W 90/11/22 | add ftapih parameter to ftniinfo(), ftnsinfo()
  | bf  5.0W 90/11/14 | CDECL ftnprntf, ftnscanf
  | ah  5.0U 90/10/22 | Improved static catalog stubs
  | ah  5.0U 90/10/16 | Stub out static catalog routines
  | bf  5.0D 90/09/20 | move ftnsinfo again, to permit ftsipi link
  | bf  5.0D 90/09/18 | move ftnsinfo definition; add ftnver (90091301)
  | DG	5.0D 90/08/15 | ftnfhloc: node, nfid parms must be (char FTDLFAR **)
  | bf  5.0D 90/08/10 | add ftncdrop
  | pbc 5.0D 90/08/08 | add FTAPIH parameter to ftnterm
  | bf  5.0D 90/08/07 | add FTAPIH parameter to ftnconn,ftnucon,ftnfhloc
  | pbc 5.0D 90/06/22 | use apih instead of ftapih as function argument
  | DG	5.0C 90/06/01 | FTADSSLOC
  | DG	5.0C 90/05/22 | use FTSTRUC instead of FTCINFO if !ftctyp_h
  | KRL 5.0C 90/05/09 | ANSI function declarations
  | DG	5.0C 90/03/29 | FTADSCATS; ignore FTADSMSG unless FTADSINDX (90032704)
  | bf  5.0C 90/03/26 | add ftninit; drop keep from ftnconn; add drop to ftnucon
  | bf  5.0B 90/01/09 | stub ftsvloop if FTADSII
  | bf  5.0B 90/01/08 | stub ftnsinfo,ftniinfo if FTADSRNET
  | KRL 5.0A 89/12/12 | int ftgmclos(ftapih)
  | bf  5.0A 89/12/08 | ftnsinfo()
  | ah  5.0A 89/12/04 | aopt added to ftbyopen args #89113003
  | DG	5.0A 89/12/01 | fticntab, ftudjs for MS-Windows
  | bf  5.0  89/12/01 | change ftcfin parameter from unsigned to FTFID
  | bf  5.0  89/11/28 | ftnpeek, ftbiresm, and catalog update stubs
  | DG	5.0  89/11/24 | ftsmclse()
  | bf  5.0  89/11/09 | FTADSINDX
  | DG	5.0  89/10/12 | ftsiccls()
  | AH  5.0  89/09/27 | ftivcati() -- put FTFB param back in
  | KRL 5.0  89/09/18 | ftivcati() -- drop FTFB param
  | RL	5.0  89/09/13 | ftgmess(),ftgmopen(),ftgmclos(),ftgnmess(),...
  | DG	5.0  89/08/22 | ftsiinit(), ftsiterm(), ftbyopen(), ftisbi()
  |-------------------|---------------------------------

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */

/* NOTE: This file is intended to be included by mainlines of programs
	 which call ftapiini() and want to eliminate the code and data
	 space occupied by unwanted API services, as opposed to merely
	 suppressing the initialization of these services at run-time.

	 The procedure is as follows:

	 (1) In the source module containing the invocation of ftapiini(),
	     define one or more of the following symbols to disable
	     the noted service:

		FTADSSRCH	search engine
		FTADSIOPN	immediate index open
		FTADSIUPD	immediate index update
		FTADSRNET	remote services in a FT/NET environment
		FTADSINDX	periodic indexing
		FTADSMSG	message file and formatter (requires FTADSINDX)
		FTADSCATS	catalog scanning for null query
		FTADSSLOC	searching local collections
		FTADSRANK*	ranking
		FTADSVRNT*	variant generation
		FTADSTHES*	thesaurus lookup
		FTADSMODF	collection modification (write,update,delete)
		FTADSBKCM	backward compatibility (ftpiinit())
	
	     *only meaningful if FTADSSLOC is NOT defined

	 (2) Include this file (ftads.h) following the chosen definitions
	     and the other major header files.  For example,

		#include "ftdefs.h"
		#define	FTADSSRCH
		#include "ftads.h"

	 If a service is disabled with this header file, it is not
	 necessary to also suppress it at run-time with ftapiini().
	 However, some memory might be conserved by doing so.
*/

#ifndef	FTDLENV

	/* search engine */

#ifdef	FTADSSRCH
int PASCAL	ftsiinit(apih,flags)
FTAPIH		apih;
unsigned	flags;
{
#ifdef	mac
FTPRAGMA (unused, (apih,flags))
#endif
	FTWHO	who = (FTWHO)1139;

	ftentry();

	ftreterr(-1,FTENOSRV);
}

int PASCAL	ftsiterm(apih)
FTAPIH		apih;
{
#ifdef	mac
FTPRAGMA (unused, (apih))
#endif
	FTWHO	who = (FTWHO)1140;

	ftentry();

	ftreterr(-1,FTENOSRV);
}

#ifdef	FTLINT
int PASCAL	ftsiccls(FTAPIH apih, FTBH bh)
#else
int PASCAL	ftsiccls(apih,bh)
FTAPIH		apih;
FTBH		bh;
#endif
{
#ifdef	mac
FTPRAGMA (unused, (apih,bh))
#endif
	FTWHO	who = (FTWHO)1185;

	ftentry();

	ftreterr(-1,FTENOSRV);
}

int FTAPIFUN	ftsmopen(apih, session, mode)
FTAPIH		apih;
char FAR	*session;
unsigned	mode;
{
#ifdef	mac
FTPRAGMA (unused, (apih,session,mode))
#endif
	FTWHO	who = (FTWHO)1171;

	ftentry();

	ftreterr(-1,FTENOSRV);
}

int PASCAL	ftsmclse(apih)
FTAPIH		apih;
{
#ifdef	mac
FTPRAGMA (unused, (apih))
#endif
	FTWHO	who = (FTWHO)1172;

	ftentry();

	ftreterr(-1,FTENOSRV);
}


#ifdef	FTHNET
#define	FTADSSI
#endif

#else	/*!FTADSSRCH*/

#ifdef	FTADSSLOC
#ifdef	FTADSCATS
/* FTADSCATS is redundant if FTADSSLOC is defined */
#undef	FTADSCATS
#endif

#ifndef	FTADSMODF
/* ftcdflsh,ftcvset are stubbed by FTADSMODF */
#define	FTADSMODF
#endif

#ifndef	ftfb_h
#include "ftfb.h"
#endif

int PASCAL	ftsbkcls(bk,sip)
char		*bk;
FTSTRUC NEAR	*sip;
{
	FTWHO		who = (FTWHO)663;

	ftentry();
	ftreterr(-1,FTENOSRV);
}

#ifdef	FTLINT
int PASCAL	ftsisbeg(FTAPIH apih,FTBH bh,FTBH mbh,unsigned colndx)
#else
int PASCAL	ftsisbeg(apih,bh,mbh,colndx)
FTAPIH		apih;
FTBH		bh,mbh;
unsigned	colndx;
#endif
{
	FTWHO	who = (FTWHO)1143;

	ftentry();
	ftreterr(-1,FTENOSRV);
}

int PASCAL	ftsisend(apih,sbp)
FTAPIH		apih;
FTSTRUC FAR	*sbp;
{
	FTWHO	who = (FTWHO)1144;

	ftentry();
	ftreterr(-1,FTENOSRV);
}

#ifdef	FTLINT
int PASCAL	ftsiswat(FTAPIH apih, FTSTRUC FAR *sbp, unsigned retafter,
		int cancel, FTBH currbh)
#else
int PASCAL	ftsiswat(apih,sbp,retafter,cancel,currbh)
FTAPIH		apih;
FTSTRUC FAR	*sbp;
unsigned	retafter;	/* return interval, in ms */
int		cancel;		/* cancel search if non-zero */
FTBH		currbh;		/* bh of current coll. (used in ftsrwcid) */
#endif
{
	FTWHO		who = (FTWHO)1161;

	ftentry();
	ftreterr(-1,FTENOSRV);
}

int PASCAL	ftsirsrt(apih, sb, cancel, retafter, reenter)
FTAPIH		apih;
FTSTRUC		*sb;
int		cancel;		/* cancel search if non-zero */
unsigned	retafter;	/* return interval, in ms */
int		reenter;	/* sort being reentered */
{
	FTWHO		who = (FTWHO)1176;

	ftentry();
	ftreterr(-1,FTENOSRV);
}

	/* formerly: ftnfgfnd() */
int	PASCAL	ftfgsl(apih,cname,fig)
FTAPIH		apih;
char	FAR	*cname;
#ifdef	ftbtyp_h
FTFIG	FAR	*fig;
#else	/*!ftbtyp_h*/
FTSTRUC	FAR	*fig;
#endif	/*!ftbtyp_h*/
{
	FTWHO		who = (FTWHO)627;

	ftentry();
	ftreterr(-1,FTENOSRV);
}

int		ftcvclse(cviob,b0)
FTSTRUC		*cviob;
FTSTRUC		*b0;
{
	FTWHO		who = (FTWHO)69;

	ftentry();
	ftreterr(-1,FTENOSRV);
}

int		ftcddel(cviob,b0,cid)
FTSTRUC		*cviob;
FTSTRUC		*b0;
FTCID		cid;
{
	FTWHO		who = (FTWHO)70;

	ftentry();
	ftreterr(-1,FTENOSRV);
}

int		ftcdfind(cviob,b0,name,cest)
FTSTRUC		*cviob;
FTSTRUC		*b0;
char	FAR	*name;
FTSTRUC	FAR	*cest;
{
	FTWHO		who = (FTWHO)82;

	ftentry();
	ftreterr(-1,FTENOSRV);
}

int		ftcdgnxt(fbp,rfid,vcc,count,datevcc)
FTFB	FAR	*fbp;
FTFID	FAR	*rfid;
FTVCC	FAR	*vcc;
USHORT	FAR	*count;
int		datevcc;
{
	FTWHO		who = (FTWHO)1073;

	ftentry();
	ftreterr(-1,FTENOSRV);
}

int		ftcdhndl(fbp,bh)
FTFB	FAR	*fbp;
FTBH		bh;
{
	FTWHO		who = (FTWHO)1076;

	ftentry();
	ftreterr(-1,FTENOSRV);
}

int		ftcvinit(apih,catfn,mapfn,catparms,yndsz)
FTAPIH		apih;
char	FAR	*catfn;
char	FAR	*mapfn;
long	FAR	*catparms;
long		yndsz;
{
	FTWHO		who = (FTWHO)73;

	ftentry();
	ftreterr(-1,FTENOSRV);
}

unsigned	ftcviobsz()
{
	return(0);
}

int		ftcvopen(apih,catfn,mapfn,cviob,b0,mode)
FTAPIH		apih;
char	FAR	*catfn;
char	FAR	*mapfn;
FTSTRUC		*cviob;
FTSTRUC		*b0;
int		mode;
{
	FTWHO		who = (FTWHO)75;

	ftentry();
	ftreterr(-1,FTENOSRV);
}

int		ftcdpnxt(fbp,fid,count)
FTFB	FAR	*fbp;
FTFID		fid;
USHORT		*count;
{
	FTWHO		who = (FTWHO)1077;

	ftentry();
	ftreterr(-1,FTENOSRV);
}

int		ftcdrd(cviob,b0,fbp,cest,list,listsz,mode,lock)
FTSTRUC		*cviob;
FTSTRUC		*b0;
FTFB	FAR	*fbp;
FTSTRUC	FAR	*cest;
FTFID	FAR	*list;
int		listsz;
int		mode;
int		lock;
{
	FTWHO		who = (FTWHO)1082;

	ftentry();
	ftreterr(-1,FTENOSRV);
}

int		ftcvrm(apih,catfn,mapfn)
FTAPIH		apih;
char	FAR	*catfn;
char	FAR	*mapfn;
{
	FTWHO		who = (FTWHO)77;

	ftentry();
	ftreterr(-1,FTENOSRV);
}

int		ftcdsget(cviob,b0,cest,lock)
FTSTRUC		*cviob;
FTSTRUC		*b0;
FTSTRUC	FAR	*cest;
int		lock;
{
	FTWHO		who = (FTWHO)80;

	ftentry();
	ftreterr(-1,FTENOSRV);
}

int		ftcdsput(cviob,b0,cest)
FTSTRUC		*cviob;
FTSTRUC		*b0;
FTSTRUC	FAR	*cest;
{
	FTWHO		who = (FTWHO)81;

	ftentry();
	ftreterr(-1,FTENOSRV);
}

int		ftcvstat(cviob,b0,stid,area)
FTSTRUC		*cviob;
FTSTRUC		*b0;
int		stid;
char	FAR	*area;
{
	FTWHO		who = (FTWHO)83;

	ftentry();
	ftreterr(-1,FTENOSRV);
}

int		ftcdwr(cviob,b0,fbp,cest,name,maxsubd,mode)
FTSTRUC		*cviob;
FTSTRUC		*b0;
FTFB	FAR	*fbp;
FTSTRUC	FAR	*cest;
char	FAR	*name;
int		maxsubd;
int		mode;
{
	FTWHO		who = (FTWHO)1085;

	ftentry();
	ftreterr(-1,FTENOSRV);
}
#else	/*!FTADSSLOC*/

#ifdef	FTADSRANK
long PASCAL	ftsrgdat(n)
FTSTRUC	FAR	*n;
{
	return(0L);
}

long PASCAL	ftsrkqt(apih,n)
FTAPIH		apih;
FTSTRUC	FAR	*n;
{
	return(0L);
}

long PASCAL	ftsrkocc(apih,n)
FTAPIH		apih;
FTSTRUC	FAR	*n;
{
	return(0L);
}

long PASCAL	ftsrkst(sip,n)
FTSTRUC	NEAR	*sip;
FTSTRUC	FAR	*n;
{
	return(0L);
}

int		ftqraset(f,ps)
FTSTRUC		*f;
char		*ps;
{
	return(0);
}

int	PASCAL	ftsirsrt(apih,sb,cancel,retafter,reenter)
FTAPIH		apih;
FTSTRUC		*sb;
int		cancel;
unsigned	retafter;
int		reenter;
{
	return(0);
}
#endif	/*FTADSRANK*/

#ifdef	FTADSVRNT
unsigned	ftqlrsiz(apih)
FTAPIH		apih;
{
	return(0);
}

int		ftqlini(apih,areap)
FTAPIH		apih;
char		*areap;
{
	return(0);
}

int		ftqlend(apih,areap)
FTAPIH		apih;
char		*areap;
{
	return(0);
}

int		ftqlmset(apih,areap)
FTAPIH		apih;
char		*areap;
{
	return(0);
}

int		ftqlmadd(apih,areap,st)
FTAPIH		apih;
char		*areap;
char		*st;
{
	return(0);
}

void		ftqlmrst(apih,areap)
FTAPIH		apih;
char		*areap;
{
	return;
}

int	PASCAL	ftqlmtck(apih,areap,smpp)
FTAPIH		apih;
char		*areap;
FTSTRUC		*smpp;
{
	return(0);
}

int	PASCAL	ftqlmip(apih,areap,f,qip)
FTAPIH		apih;
char		*areap;
FTSTRUC		*f;
FTSTRUC	FAR	*qip;
{
	return(0);
}

int		ftqlem(cregmw,crgsb,crgst,irgsb,irgst)
int		cregmw;
FTSTRUC		**crgsb;
FTSTRUC		**crgst;
FTSTRUC		**irgsb;
FTSTRUC		**irgst;
{
	return(0);
}
#endif	/*FTADSVRNT*/

#ifdef	FTADSTHES
void	ftqthini()
{
	return;
}

#ifdef FTLINT
void	fthltfre(FTSTRUC *tfp)
#else
void	fthltfre(tfp)
FTSTRUC	*tfp;
#endif
{
	return;
}

#ifdef FTLINT
int	ftqthtrm(FTAPIH ftapih)
#else
int	ftqthtrm(ftapih)
FTAPIH	ftapih;
#endif
{
	return;
}

int		ftqthrst()
{
	return(0);
}

int		ftqthset(apih)
FTAPIH		apih;
{
	return(0);
}

int		ftqthadd(apih,sfn)
FTAPIH		apih;
char		*sfn;
{
	return(0);
}

int	PASCAL	ftqtsuf(apih,fs,cpppret)
FTAPIH		apih;
char	FAR	*fs;
char FAR * FAR * FAR *cpppret;
{
	return(0);
}

int	PASCAL	ftqtsyn(apih,fs,cpppret)
FTAPIH		apih;
char	FAR	*fs;
char FAR * FAR * FAR *cpppret;
{
	return(0);
}

int		ftqterm(cregmw,crgsb,crgst,irgsb,irgst)
int		cregmw;
FTSTRUC		**crgsb;
FTSTRUC		**crgst;
FTSTRUC		**irgsb;
FTSTRUC		**irgst;
{
	return(0);
}
#endif	/*FTADSTHES*/

#endif	/*!FTADSSLOC*/

#ifdef	FTADSMODF
#ifndef	FTADSSLOC
int		ftcdblck(cviob)
FTSTRUC		*cviob;
{
	return(0);
}

int		ftcdbulk(cviob)
FTSTRUC		*cviob;
{
	return(0);
}

int	PASCAL	ftrflock(apih,rfd,func,size)
FTAPIH		apih;
int		rfd;
int		func;
long		size;
{
	return(0);
}

#ifdef	FTHMSDOS
int	FTAPIFUN ftuisnet()
{
	return(0);
}
#endif	/*FTHMSDOS*/
#endif	/*!FTADSSLOC*/

int	FTAPIFUN ftcvset(cviob,b0,stid,area)
FTSTRUC		*cviob;
FTSTRUC		*b0;
int		stid;
char	FAR	*area;
{
	FTWHO		who = (FTWHO)79;

	ftentry();
	ftreterr(-1,FTENOSRV);
}

int	FTAPIFUN ftcdflsh(cviob,unlock)
FTSTRUC		*cviob;
int		unlock;
{
	return(0);
}
#endif	/*FTADSMODF*/


#ifdef	FTADSCATS
int PASCAL	ftsicbeg(sip)
FTSTRUC NEAR	*sip;
{
#ifdef	mac
FTPRAGMA (unused, (sip))
#endif
	FTWHO	who = (FTWHO)1304;

	ftentry();

	ftreterr(-1,FTENOSRV);
}

int PASCAL	ftsicwat(sip)
FTSTRUC NEAR	*sip;
{
#ifdef	mac
FTPRAGMA (unused, (sip))
#endif
	FTWHO	who = (FTWHO)1304;

	ftentry();

	ftreterr(-1,FTENOSRV);
}

int PASCAL	ftsicend(sip)
FTSTRUC NEAR	*sip;
{
#ifdef	mac
FTPRAGMA (unused, (sip))
#endif
	FTWHO	who = (FTWHO)1304;

	ftentry();

	ftreterr(-1,FTENOSRV);
}
#endif	/*FTADSCATS*/

#endif	/*!FTADSSRCH*/

	/* immediate index open */

#ifdef	FTHDYX
#ifdef	FTADSIOPN
int	ftbyopen(char *dyxfn, char *dctfn, FTSTRUC *bb, int mode, int aopt)
{
#ifdef	mac
FTPRAGMA (unused, (dyxfn,dctfn,bb,mode,aopt))
#endif
	FTWHO	who = (FTWHO)44;

	ftentry();

	ftreterr(-1,FTENOSRV);
}

int	ftbyclse(FTSTRUC *yiob, int mode)
{
#ifdef	mac
FTPRAGMA (unused, (yiob,mode))
#endif
	FTWHO	who = (FTWHO)42;

	ftentry();

	ftreterr(-1,FTENOSRV);
}
#endif	/*FTADSIOPN*/

	/* immediate index update */

#ifdef	FTADSIUPD
int	ftivcati(FTSTRUC *vm,
#ifdef	ftctyp_h
FTCINFO *pcinfo,
#else
FTSTRUC *pcinfo,
#endif
#ifdef	ftfb_h
FTFB	FAR *fb,
#else
FTSTRUC	FAR *fb,
#endif
FTVCC	voff)
{
#ifdef	mac
FTPRAGMA (unused, (vm,fb,pcinfo,voff))
#endif
	FTWHO	who = (FTWHO)187;

	ftentry();

	ftreterr(-1, FTENOSRV);
}

int	ftivtxti(FTSTRUC *vm,FTSTRUC	*fb,
#ifdef	ftctyp_h
FTCINFO *pcinfo,
#else
FTSTRUC *pcinfo,
#endif
FTVCC	voff)
{
#ifdef	mac
FTPRAGMA (unused, (vm,fb,pcinfo,voff))
#endif
	FTWHO	who = (FTWHO)192;

	ftentry();

	ftreterr(-1, FTENOSRV);
}

int	ftybtupd(FTSTRUC cwvec, long cwvlen, char *yiob)
{
#ifdef	mac
FTPRAGMA (unused, (cwvec,cwvlen,yiob))
#endif
	FTWHO	who = (FTWHO)462;

	ftentry();

	ftreterr(-1,FTENOSRV);
}

int	ftisbi(FTSTRUC pisb)
{
#ifdef	mac
FTPRAGMA (unused, (pisb))
#endif
	FTWHO	who = (FTWHO)179;

	ftentry();

        ftreterr(-1, FTENOSRV);
}

int	ftisbc(
int	(*putfun)(),
char	*arg)
{
#ifdef	mac
FTPRAGMA (unused, (putfun,arg))
#endif
	FTWHO	who = (FTWHO)176;

	ftentry();

        ftreterr(-1, FTENOSRV);
}

int	ftisbe()
{
	FTWHO	who = (FTWHO)177;

	ftentry();

        ftreterr(0, FTENOSRV);
}
#endif	/*!FTADSIUPD*/
#endif	/*FTHDYX*/

	/* remote services in a FT/NET environment */

#ifdef	FTADSRNET
#define FTADSII

/* ftnlunk() -- indicate to caller whether networking code is lunk in */

int FTEXPORT PASCAL ftnlunk()
{
	return(0);
}

#ifdef	FTHNET
#include "ftnfb.h"

int	ftninit(FTAPIH apih)
{
#ifdef	mac
FTPRAGMA (unused, (apih))
#endif
	return(0);
}

FTNFH FTAPIFUN ftnconn(FTAPIH apih,char	FAR *node,char	FAR *nfid)
{
#ifdef	mac
FTPRAGMA (unused, (apih,node,nfid))
#endif
	FTWHO	who = (FTWHO)223;

	ftentry();

	ftreterr((FTNFH)NULL,FTENOSRV);
}

#ifdef	FTLINT
int FTAPIFUN ftnucon(FTAPIH apih,FTNFH nfh, int drop)
#else
int FTAPIFUN ftnucon(apih,nfh,drop)
FTAPIH 	apih;
FTNFH	nfh;
int	drop;
#endif
{
#ifdef	mac
FTPRAGMA (unused, (apih,nfh,drop))
#endif
	FTWHO	who = (FTWHO)258;

	ftentry();

	ftreterr(-1,FTENOSRV);
}

FTNFH FTAPIFUN ftnopen(
FTAPIH		apih,
char	FAR	*node,
char	FAR	*nfid)
{
#ifdef	mac
FTPRAGMA (unused, (node,nfid))
#endif
	FTWHO	who = (FTWHO)242;

	ftentry();

	ftreterr((FTNFH)NULL,FTENOSRV);
}

int FTAPIFUN ftnpeek(apih,nfh)
FTAPIH	apih;
FTNFH	nfh;
{
#ifdef	mac
FTPRAGMA (unused, (nfh))
#endif
	FTWHO	who = (FTWHO)31;

	ftentry();

	ftreterr(-1,FTENOSRV);
}

#ifdef	FTHANSIC
int CDECL FTLOADDS FTEXPORT ftnprntf(FTAPIH apih, FTNFH nfh, char FAR *f, ...)
#else
/*VARARGS3*/
int CDECL FTLOADDS FTEXPORT ftnprntf(apih,nfh,f,ip)
FTAPIH	apih;
FTNFH	nfh;
char	FAR *f;
char	FAR *ip;
#endif
{
#ifdef	mac
FTPRAGMA (unused, (nfh,f,ip))
#endif
	FTWHO	who = (FTWHO)244;

	ftentry();

	ftreterr(-1,FTENOSRV);
}

#ifdef	FTHANSIC
int CDECL FTLOADDS FTEXPORT ftnscanf(FTAPIH apih, FTNFH nfh, char FAR *f, ...)
#else
/*VARARGS3*/
int CDECL FTLOADDS FTEXPORT ftnscanf(apih,nfh,f,ip)
FTAPIH	apih;
FTNFH	nfh;
char	FAR *f;
char	FAR *ip;
#endif
{
#ifdef	mac
FTPRAGMA (unused, (nfh,f,ip))
#endif
	FTWHO	who = (FTWHO)245;

	ftentry();

	ftreterr(-1,FTENOSRV);
}

int	ftnfind(apih,cname,fig)
FTAPIH		apih;
char	FAR	*cname;
#ifdef	ftbtyp_h
FTFIG	FAR	*fig;
#else	/*!ftbtyp_h*/
FTSTRUC	FAR	*fig;
#endif	/*!ftbtyp_h*/
{
#ifdef	mac
FTPRAGMA (unused, (cname,fig))
#endif
	FTWHO	who = (FTWHO)237;

	ftentry();

	ftreterr(-1,FTENOTDB);
}

#ifdef	FTLINT
int ftnfhloc(FTAPIH apih,FTNFH nfh, char **node, char **nfid)
#else
int	ftnfhloc(apih,nfh,node,nfid)
FTAPIH 	apih;
FTNFH	nfh;
char	**node;
char	**nfid;
#endif
{
#ifdef	mac
FTPRAGMA (unused, (apih,nfh,node,nfid))
#endif
	FTWHO	who = (FTWHO)512;

	ftentry();

	ftreterr(-1,FTENOSRV);
}

#ifdef	FTLINT
int	ftnterm(FTAPIH apih)
#else
int	ftnterm(apih)
FTAPIH 	apih;
#endif
{
#ifdef	mac
FTPRAGMA (unused, (apih))
#endif
	return(0);
}

#ifdef	FTLINT
int	ftncdrop(FTAPIH apih)
#else
int	ftncdrop(apih)
FTAPIH 	apih;
#endif
{
#ifdef	mac
FTPRAGMA (unused, (apih))
#endif
	return(0);
}

FTNFILTT *ftnftabp()
{
	FTWHO	who = (FTWHO)568;

	ftentry();

	ftreterr((FTNFILTT *)NULL,FTENOSRV);
}

unsigned ftnpacksz(apih,nfh)
FTAPIH	apih;
FTNFH	nfh;
{
#ifdef	mac
FTPRAGMA (unused, (nfh))
#endif
	FTWHO	who = (FTWHO)888;

	ftentry();

	ftreterr(-1,FTENOSRV);
}

int	ftnread(apih,nfh,buf,len)
FTAPIH		apih;
register FTNFH	nfh;
char	FAR	*buf;
int		len;
{
#ifdef	mac
FTPRAGMA (unused, (nfh,buf,len))
#endif
	FTWHO	who = (FTWHO)928;

	ftentry();

	ftreterr(-1,FTENOSRV);
}

USHORT ftnver(apih,nfh)
FTAPIH		apih;
FTREG FTNFH	nfh;
{
#ifdef	mac
FTPRAGMA (unused, (nfh))
#endif
	FTWHO		who = (FTWHO)931;

	ftentry();

	ftreterr(-1,FTENOSRV);
}
#endif	/*FTHNET*/
#endif	/*FTADSRNET*/

#ifdef	FTADSINDX
#define FTADSII
#ifndef	ftityp_h
#include "ftitypes.h"
#endif

int FTAPIFXW ftbistrt(
FTAPIH	apih,
char	FAR *cname,
char	FAR *ftprog,
long	textlim,
long	srtbsiz,
long	flags,
long	msgint,
FTIH	FAR *ih)
{
#ifdef	mac
FTPRAGMA (unused, (apih,cname,ftprog,textlim,srtbsiz,flags,msgint,ih))
#endif
	FTWHO	who = (FTWHO)1206;

	ftentry();

	ftreterr(-1,FTENOSRV);
}

#ifdef	FTLINT
int FTAPIFXW ftbiresm(FTAPIH apih, FTIH ih, long retafter, int cancel,
	    FTIDATA FAR *idata)
#else
int FTAPIFXW ftbiresm(apih,ih,retafter,cancel,idata)
FTAPIH	apih;
FTIH	ih;
long	retafter;
int	cancel;
FTIDATA	FAR *idata;
#endif
{
#ifdef	mac
FTPRAGMA (unused, (apih,ih,retafter,cancel,idata))
#endif
	FTWHO	who = (FTWHO)1202;

	ftentry();

	ftreterr(-1,FTENOSRV);
}

#ifndef	FTADSCUPD
/* catalog update functions are no-ops unless indexing enabled */
int FTAPIFUN fticmpmk(
FTPDP		icmh,		/* context map handle - UNUSED		    */
UCHAR	FAR	*ps,		/* FTCS parameter string of context marker  */
				/* format:  [A1[;A2]...][?]		    */
FTVCC		vcc)		/* current VCC at context marker location   */
{
	return(0);
}

int FTAPIFUN ftcfgi(FTAPIH apih)
{
#ifdef	mac
FTPRAGMA (unused, (apih))
#endif
	return(0);
}

#ifdef	FTLINT
int FTAPIFUN ftcfin(FTAPIH apih, FTFID fid)
#else
int FTAPIFUN ftcfin(
FTAPIH	apih,
FTFID	fid)
#endif
{
#ifdef	mac
FTPRAGMA (unused, (apih,fid))
#endif
	return(0);
}

int FTAPIFUN ftcfpf(
FTAPIH	apih,
int	c)
{
#ifdef	mac
FTPRAGMA (unused, (apih,c))
#endif
	return(0);
}

int FTAPIFUN ftcfte(FTAPIH apih)
{
#ifdef	mac
FTPRAGMA (unused, (apih))
#endif
	return(0);
}
#endif	/*!FTADSCUPD*/

#else	/*!FTADSINDX*/
#ifdef	FTADSMSG
/* FTADSMSG cannot be used unless FTADSINDX is defined */
#undef	FTADSMSG
#endif
#endif	/*!FTADSINDX*/
#endif	/*!FTDLENV*/

#ifdef	FTADSMSG	/* formatted output functions */
/*VARARGS2*/		/* variable argument list */
#ifdef	FTHANSIC
int	CDECL	ftuprntf(FILE *fp, char *f, ...)
#else	/*!FTHANSIC*/
#ifdef	FTLINT
int	CDECL	ftuprntf(FILE *fp, char *f, char *ip, char *ip2, char *ip3,
			char *ip4, char *ip5, char *ip6, char *ip7, char *ip8)
#else
int	CDECL	ftuprntf(fp,f,ip,ip2,ip3,ip4,ip5,ip6,ip7,ip8)
FILE		*fp;
char		*f;
char		*ip,*ip2,*ip3,*ip4,*ip5,*ip6,*ip7,*ip8;
#endif
#endif	/*!FTHANSIC*/
{
	return(0);
}

#ifdef	FTHDOSSDM
/*VARARGS2*/		/* variable argument list */
int	CDECL	ftufprtf(FILE *fp,char FAR *f,...)
{
	return(0);
}
#endif	/*FTHDOSSDM*/
#endif	/*FTADSMSG*/

	/* the following functions are in the DLL except in Windows */
#ifdef	FTDLENV
#ifndef	FTHWIN
#ifdef	FTADSMSG
#undef	FTADSMSG
#endif	/*FTADSMSG*/
#endif	/*!FTHWIN*/
#endif	/*FTDLENV*/

#ifdef	FTADSMSG	/* ftgmess functions */

/* formerly ftgmess */
char FAR * FTAPIFXW ftutgm(
FTAPIH		apih,
unsigned	messnum)
{
	return("");
}

/* formerly ftgnmess */
char FAR * FTAPIFXW ftutgn(
FTAPIH		apih,
unsigned	mno)
{
	return("");
}

/* formerly ftgmerno */
char FAR * FTAPIFXW ftutge(
FTAPIH		apih,
int		erno)
{
	return("");
}

void FTAPIFXW ftgminit()
{
}

int FTAPIFXW ftgmopen(FTAPIH apih)
{
	FTWHO		who = (FTWHO)1121;

	ftentry();

	ftreterr(-1,FTENOSRV);
}

int FTAPIFXW ftgmclos(FTAPIH apih)
{
	return(0);
}
#endif	/*FTADSMSG*/

#ifdef	FTADSBKCM
char		*ftufullp(int flags)
{
	return((char *)NULL);
}

int		ftpiinit(
unsigned	optpool,
unsigned	minpool)
{
	return(FTENOSRV);
}
#endif	/*FTADSBKCM*/

#ifndef	FTDLENV
#ifdef	FTHNET
	/* special section for async message handlers */
	/* these are stubbed if FTADSRNET is set, or if */
	/* searching/indexing/query filters */
	/* are disabled respectively */
#ifdef	FTADSSI
int	ftnsinfo(
FTAPIH		apih,
register FTNFH	nfh)
{
#ifdef	mac
FTPRAGMA (unused, (nfh))
#endif
	FTWHO		who = (FTWHO)1223;
	ftentry();

	ftreterr(-1,FTENOSRV);
}
#endif	/*FTADSSI*/

#ifdef	FTADSQI
int	ftnqinfo(
FTAPIH		apih,
register FTNFH	nfh)
{
#ifdef	mac
FTPRAGMA (unused, (nfh))
#endif
	FTWHO		who = (FTWHO)1506;
	ftentry();

	ftreterr(-1,FTENOSRV);
}
#endif	/*FTADSQI*/

#ifdef	FTADSII
#ifdef	FTHWIN
int FTEXPORT PASCAL ftniinfo(
#else	/*!FTHWIN*/
int	ftniinfo(
#endif	/*!FTHWIN*/
FTAPIH		apih,
register FTNFH	nfh)
{
#ifdef	mac
FTPRAGMA (unused, (nfh))
#endif
	FTWHO		who = (FTWHO)1204;
	ftentry();

	ftreterr(-1,FTENOSRV);
}

/* KLUGE: this stub is necessary so that client applications can link */
int	ftsvloop(
FTNFH	nfh,
int	block)
{
#ifdef	mac
FTPRAGMA (unused, (nfh,block))
#endif
	FTWHO		who = (FTWHO)250;

	ftentry();

	ftreterr(-1,FTENOSRV);
}
#endif	/*FTADSII*/
#endif	/*FTHNET*/

#ifndef FTCSTAT
#ifndef ftfb_h
#define FTFB	FTSTRUC
#endif
FTFB	FAR *ftcsrd(FTAPIH apih)
{
#ifdef	mac
FTPRAGMA (unused, (apih))
#endif
	FTWHO	who = (FTWHO)1352;

	ftentry();

	ftreterr((FTFB FAR *)NULL,FTENOSRV);
}

FTCID	ftcsncid(FTAPIH apih)
{
#ifdef	mac
FTPRAGMA (unused, (apih))
#endif
	FTWHO	who = (FTWHO)1361;

	ftentry();

	ftreterr(-1,FTENOSRV);
}

int	ftcssget(FTAPIH apih)
{
#ifdef	mac
FTPRAGMA (unused, (apih))
#endif
	FTWHO	who = (FTWHO)1355;

	ftentry();

	ftreterr(-1,FTENOSRV);
}

int ftcsrbin()
{
	return(-1);
}
#endif
#endif	/*!FTDLENV*/
