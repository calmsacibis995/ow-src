/*static char *sccsid="@(#) fthio.h (1.4)  14:21:06 07/08/97";			

   fthio.h -- host operating system's differences flags			

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------		
  | crs 5.1C 92/02/19 | SunOS/SVR4: ifndef FTHSVR4 extern char     *calloc(); 
  | crs 5.1C 92/02/17 | SunOS/SVR4: ifndef FTHSVR4 include "strings.h" 
  | swr 5.1B 91/12/06 | crs: ix/SVR4: disable FTCHKSPC (KLUGE)
  | ah  5.1B 91/11/20 | SunOS: Move calloc declaration after #include malloc.h
  | swr 5.1B 91/11/14 | 91082703: add FTDOSDLY
  | StM 5.1B 91/10/16 | 91100906: char *calloc() if sun not 4.1.1
  | MM  5.1B 91/10/08 | ftuminit() prototype
  | hjb 5.1B 91/10/01 | ftugenv() prototype
  | rdc 5.1B 91/09/25 | Added ftuqhndl()
  | StM 5.1B 91/09/16 | mrd: osf/1 port
  | cps 5.1B 91/09/04 | Changes for Windows VSS (Define FTSVSS for FTHWIN)
  | StM 5.1B 91/08/30 | ultrix: declare char *calloc() (not in malloc.h)
  | swr 5.1B 91/08/27 | 91021401: Mac: new segmentation scheme
  | swr 5.1B 91/08/27 | 91011401: Mac: ftmalloc et al.; def'd FTHNDL
  | ded 5.1B 91/08/21 | Define FTLFCNTL (locking using fcntl) for sun
  | StM 5.1B 91/08/20 | 91070503: include <malloc.h> if __hpux or ultrix
  | StM 5.1B 91/07/05 | Define FTSVSS for ultrix
  | cps 5.1B 91/07/03 | Define FTSVSS for FTHOS2
  | wtr 5.1A 91/05/14 | Use malloc.h & strings.h in sun environment
  | bf  5.0E 91/05/01 | SunOS/SVR4: disable FTCHKSPC (KLUGE)
  | bf  5.1A 91/04/18 | don't declare memcpy; set FTHANSIC for DOS, OS/2
  | bf  5.0E 91/04/15 | SunOS/SVR4 port
  | bf  5.1W 91/04/11 | prototypes for ftufree,ftucache,ftureuse
  | ah  5.1A 91/04/10 | Define FTSVSS in hp_ux section
  | bf  5.1A 91/04/02 | define FTAPIFUN etc. for DOS; drop FTUCFSTR
  | ah  5.1A 91/03/13 | Force FTLINT on for the mac.
  | bf  5.1W 91/03/09 | ftumalloc prototypes: use FTWHO instead of char *
  | hjb 5.1W 91/04/03 | 91030402: FTWHO is char NEAR *
  | swr 5.1A 91/02/21 | 90121701: Silicon Graphics port (sgi)
  | swr 5.1A 91/02/21 | 90121001: DECstation 5000 port (vax -> ultrix)
  | swr 5.1A 91/02/21 | 90110201: IBM RS 6000 port (aix)
  | cps 5.1A 91/02/15 | Define FTSVSS in sun section
  | bf  5.0W 91/02/04 | drop FTDLFAR
  | hjb	5.0W 91/02/04 | WIN LDM: ftumfalx(),ftumfcsx() now in import library
  | hjb 5.0W 91/01/09 | FTAPIFUN ftumffre(), etc. (WIN LDM)
  | bf  5.0W 91/01/08 | FTEXPORT ftumn* [hjb adds: PASCAL]
  | bf  5.0W 90/12/27 | FTDLLIB: don't expand FTDBG,ftentry etc.; ftugenv() FAR*
  | hjb	5.0W 90/12/21 | FTHPROTO; FTAPIFUN ftucfstr
  | bf  5.0W 90/12/12 | msdos: don't include malloc.h, fcntl.h
  | bf  5.0W 90/11/13 | CDECL ftufprtf; read, write parameters are void *
  | bf  5.0W 90/11/09 | FTDLWIN
  | bf  5.0W 90/11/06 | C 6.0 keywords; move some FTSIG* stuff to ftsignal.h
  | bf  5.0D 90/09/28 | CONST for all environments
  | hjb 5.0D 90/09/25 | Windows LDM generic allocators -> far memory	
  | PW  5.0D 90/09/13 | FTHNOSLT, FTLOCKF for tower			
  | DAT 5.0D 90/08/11 | SM: VM port					
  | swr 5.0D 90/08/10 | Mac: removed  FTMACAPL (now a global)		
  | swr 5.0D 90/07/24 | Mac: removed Mac-specific defines; added FTMACAPL
  | rtk 5.0D 90/07/16 | avoid double definition of _WINDOWS (#90071301)
  | hjb 5.0D 90/07/04 | changes for Windows LDM (allocators; NULL; ...)	
  | ah  5.0D 90/06/28 | remove FTBIGCAT definitions			
  | bf  5.0D 90/06/28 | FTHNOREG					
  | KRL 5.0C 90/06/08 | FTHXLM						
  | bf  5.0C 90/06/05 | define NULL for Windows + C 6.0			
  | swr 5.0C 90/06/01 | add FTPRAGMA macro				
  | KRL 5.0C 90/05/30 | msdos: drop DIRSIZ, struct direct		
  | swr 5.0C 90/05/23 | Mac: turned on FTHNET				
  | KRL 5.0C 90/05/21 | DGUX: FTLINT forced if __STDC__ or __STDH__	
  | bf  5.0C 90/05/17 | DOS: change USHORT from define to typedef	
  | KRL 5.0C 90/02/03 | __hpux: __STDC__				
  | DG  5.0C 90/05/01 | ftubrk() ftufix() prototypes to fthandle.h	
  | DG	5.0C 90/04/17 | MS-WIN:_WINDOWS,_WINDLL; MS-DOS:drop extern errno
  | rdc 5.0C 90/04/10 | i386: #define FTHNOSLT and FTHNFS		
  | DG	5.0C 90/04/09 | move FTVADMIN to ftypes.h			
  | rdc 5.0C 90/04/03 | ATT 3B2 & 3B5: #define FTHNOSLT 		
  | PW  5.0C 90/03/26 | prototypes for ftuindx(),fturindx() to fthandle.h
  | DG	5.0C 90/03/23 | MS-DOS: add prototypes for strchr(), strrchr()	
  | PW  5.0C 90/03/22 | add prototypes for ftuindx(), fturindx()	
  | PW  5.0C 90/03/22 | change index,rindex to ftuindx,fturindx 	
  | swr 5.0B 90/02/27 | Mac: added DIRSIZ, MAXNAMLEN			
  | KRL 5.0A 89/12/21 | FTHWIN: don't #define ftubrchk() (ftwindex.c)	
  | bf  5.0A 89/12/21 | FTHNOSIG: ftubrchk: don't reference ftubrflg	
  | swr 5.0A 89/12/19 | Mac: added const for file/dir names		
  | bf  5.0A 89/12/15 | redefine ftusleep				
  | DG  5.0A 89/12/12 | fix FTFDBG					
  | bf  5.0A 89/12/08 | FTHNOSIG					
  | cs  5.0  89/11/28 | increase FTNFILE for Sun to 64			
  | bf  5.0  89/11/27 | FTMVOID						
  | swr 5.0  89/11/16 | Macintosh MPW port				
  | KRL 5.0  89/11/11 | ftfr...() to fthandle.h				
  | KRL 5.0  89/11/11 | ftudatr() ftufach() ftufatr() to ftutypes.h	
  | bf  5.0  89/11/10 | MS-DOS: change ftmnfix,ftmnbrk macros		
  | bf  5.0  89/11/09 | remove ftipmsg;drop FTHWS;default FTHNDL int*	
  | DG  5.0  89/10/26 | FTHDOSSDM, ftuprntf; drop FTHVDM		
  | bf  4.6C 89/10/20 | AViiON (DGUX)					
  | rtk 5.0  89/10/17 | add comment delimiters to #else	!FTHOS2		
  | rl  5.0  89/10/17 | define FTHNET for FTHUNIX 			
  | KRL 5.0  89/10/13 | ftmmnalc ftmnfre ftmnfix ftmnbrk; drop ftmcallc	
  | KRL 5.0  89/10/12 | force FTLINT for MSC				
  | KRL 5.0  89/09/15 | drop CRDS (unos), PRO VENIX			
  | rtk 5.0  89/09/06 | add ftuctoi() for UCHAR -> int conversion	
  | KRL 5.0  89/09/01 | drop #define ftretok(v) allmem bldmem		
  | DG	5.0  89/08/28 | drop ftfrrd(), ftfrwr()				
  | DG	5.0  89/08/22 | FTHSSPA replaces FTHVDM				
  | KRL 5.0  89/08/09 | __hpux: don't need FTHFHDR			
  | SM  5.0  89/08/04 | vms: remove typedef struct _pointlock		
  | DG	5.0  89/07/25 | MS-Windows: move ftumnrlc() to ftgfuncs.h	
  | DG	5.0  89/07/10 | API repackaging					
  | bf  4.6  89/05/19 | drop FTLKINC (purpose now served by ftflock.h)	
  | DG	4.6  89/05/03 | need ftfrndx if FTDLENV, not just FTDLUSR	
  | pbc 4.6A 89/05/09 | aosvs:FTHNDL needs to be (int *)		
  | DG	4.6  89/05/03 | near memory mirrors				
  | DG	4.6  89/04/24 | MS-Windows: FTNOD for DLL NODATA modules	
  | DG	4.6  89/04/18 | FTMT; ftmmallc(), ftmcallc() for moveable memory
  | ah  4.6  89/04/11 | FTASIZE added					
  | bf  4.6  89/04/10 | FTHPOSIX					
  | KRL 4.6  89/04/05 | FTHWIN: don't #define exit			
  | pbc 4.6  89/04/02 | complete implementation of ftreallc()		
  | DG	4.6  89/01/25 | fix ftucfblk() prototype			
  | bf  4.6  89/03/28 | ftrealc(); FTPD,FTPL				
  | DG	4.6  89/02/14 | drop ftumfalc, ftumffre (see ftgmem.h)		
  | bf	4.6  89/02/13 | ftfix,ftbrk; FTHDEVEL				
  | bf	4.6  89/01/10 | FTHNDL						
  | DG	4.6  89/01/06 | FTDLFAR; neutralize FTDBGON, FTDBGOFF, FTDBGWHO	
  | rtk 4.6  88/12/28 | FTCOPFBLK					
  | bf	4.6  88/12/20 | FTLTRACK lock tracking code			
  | DG	4.6  88/12/14 | undefine FTHTIMEB for Microsoft C		
  | KRL 4.6  88/12/14 | vms: FTFR.frparam				
  | KRL 4.6  88/12/09 | vms: use execv() to avoid #define execvp execv	
  |-------------------|---------------------------------		

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */

/*  flags:

FTCHKSPC	Index space checking under System V, MSDOS
FTHALIGN	pointer alignment required (not 8086)
FTHANSIC	compiler is ANSI compliant
FTHASCII	native character set is standard ASCII
FTHBHDR		a.out.h structure is bhdr instead of exec
FTHBSD		BSD 4.2
FTHDEVEL	Development environment (set by FTFLAGS 'V')
FTHDFILT	Default filter list for reading the stoplist/message files
FTHDYX		Immediate collections supported
FTHEBCDIC	native character set is standard EBCDIC
FTHFCNTL	fcntl() system call is supported
FTHFFNSC	FTCS file name separator
FTHFGSC		configuration file string separator
FTHFHDR		new file header -- a.out magic numbers (UNIX V)
FTHFNSC		NCS file name separator ('/' in UNIX)
FTHFXTSC	FTCS filename extension prefix
FTHINS		Enable instrumented PI
FTHLEMMA	Lemma feature supported
FTHMEMCP	System V memcpy() et al. supported
FTHMSDOS	(also vms, aosvs) not UNIX at all
FTHNET		Client-server Ful/Text
FTHNFS		Environment supports Sun-NFS filesystems
FTHNOB		Disable time-bomb checking for porting purposes; this symbol
		should remain undefined for all production software.
FTHNOREG	no register declarations desired
FTHNOSIG	No support for signal functions
FTHNOVM		No support for "void main" declaration
FTHPNSC		separator character for PATH,FULPATH components (':' in UNIX)
FTHPOSIX	POSIX standard supported (in particular, termios.h exists)
FTHPROTO	compiler supports ANSI-style function prototypes
FTHSDM		small data model (big program, 64k data limit)
FTHSSPA		Search Single Process Architecture
FTHSTASS	Support for assignment of structures
FTHSYSTH	sys/time.h exists but time.h doesn't
FTHTERMIO	force use of termio.h (System III/V terminal driver)
FTHTIMEB	sys/timeb.h exists
FTHUCHAR	unsigned char data type exists
FTHULIM		System V ulimit() supported
FTHUSTAT	System V ustat() supported (required for FTCHKSPC)
FTHUTIME	System V utime() supported
FTHVFORK	vfork() system call is supported
FTHXEND		end(3c) unsupported
FTHXLM		large address space platforms
FTHXTSC		NCS filename extension prefix ('.' in UNIX)
FTIDCMARK	Enable document context marker support for indexing
FTMVOID		typedef for main functions
FTNOD		Suppress static and external data for MS-Windows DLL modules
		which use their caller's data segment
FTPRAGMA 	ANSI compiler-specific implementation of #pragma directive
UCHAR		unsigned char
USHORT		unsigned short

*/

#ifndef	fthio_h
#define fthio_h 1

#ifndef	FTHVMCMS

#ifdef __hp_osf
#define osf1	/* 91071601 new port	mrd */
#endif

#ifdef  _AIX            /*90110201*/
#define aix
#endif

#ifdef	applec
#define	mac
#undef	m68k
#undef	mc68000
#endif

#define FTHFGSC '#'

#define FTIDCMARK	1

#ifndef	BUFSIZ
#ifdef	FTHWIN
#ifdef	FTDLENV
#define FTDLWIN
#endif
#if (_MSC_VER >= 600)
#ifndef	M_I86LM
#define NULL	0	/* for C 6.0 SDM */
#else	/*!M_I86LM*/
#define	NULL	0L	/* for C 6.0 LDM */
#endif	/*!M_I86LM*/
#endif	/*(_MSC_VER >= 600)*/
#endif
#include <stdio.h>
#endif
#ifdef	_NFILE
#define FTNFILE _NFILE
#else
#define FTNFILE 20	/* maximum number of open files */
#endif
#ifdef	aosvs
#define FTNFILE 100
#endif

#define FTHLEMMA	1
#define FTHASCII	1
#define FTHSTASS	1
#define FTHSSPA		1

#if BUFSIZ == 512
#ifdef	MSDOS
#ifndef	FTLINT
#define	FTLINT
#endif

#undef	FTNFILE 	/* don't use _NFILE from stdio.h */
#ifndef	FTNOD
#define FTNFILE _nfile
extern int _cdecl _nfile;
#endif	/*!FTNOD*/
#define CDECL	_cdecl
#ifndef	NEAR		/* don't redefine NEAR, FAR, and PASCAL (Windows) */
#define NEAR	near
#define FAR	far
#define PASCAL	pascal
#endif	/*!NEAR*/
#define CONST	const
#define VOLATILE volatile
#define SIZE_T	size_t

#define	ftuindx		strchr
#define fturindx	strrchr
#define sizmem()	(long)(_memavl())
#ifndef	M_I86LM       /* !Microsoft large model program */
#define FTHSDM	1
#define	FTHDOSSDM 1
#endif
#ifdef	FTHWIN
#ifndef	_WINDOWS
#define	_WINDOWS
#endif	/*!_WINDOWS*/
#ifdef	FTDLENV 	/* dynamic linking environment */
#define FTDLSTAT	static
#ifdef	FTDLLIB		/* a DLL function */
#define	_WINDLL
#else	/*!FTDLLIB*/
#define FTDLUSR 1	/* may call DLL functions */
#endif	/*!FTDLLIB*/
#endif	/*FTDLENV*/
#endif	/*FTHWIN*/

#define LINT_ARGS
extern char _far * _far _cdecl _fstrcpy(char _far *, const char _far *);
extern long _cdecl	lseek(int, long, int);
extern int _cdecl	read(int, void *, unsigned int);
extern int _cdecl	write(int, const void *, unsigned int);
extern char * _cdecl	strcpy(char *, const char *);
extern char * _cdecl	strncpy(char *, const char *, size_t);
extern int _cdecl	strncmp(const char *, const char *, size_t);
extern char * _cdecl	strchr(const char *, int);
extern char * _cdecl	strrchr(const char *, int);
#include <stdlib.h>

#ifdef	FTHOS2
#define FTHDYX
#define FTSVSS
#ifndef	FTHNET
#define FTHNET 1
#endif
#else	/*!FTHOS2*/
#define FTHDOSONLY
#endif	/*!FTHOS2*/
#define FTHMSDOS 1
#define FTLOCKING 1
#define FTHUTIME 1
#define FTHUCHAR 1
#define FTHALIGN 1
#define FTHMEMCP
#define FTHDVSC ':'
#define FTHXTSC '.'
#define FTHFXTSC	0x2e
#define FTHFNSC '\\'
#define FTHFFNSC	0x5c
#define FTHPNSC ';'
typedef unsigned short	USHORT;
typedef unsigned char	UCHAR;
typedef long		time_t;
#define FTBASED(x)	_based(x)
#define FTOFF(x)	(*((unsigned _far *)&(x)))
#define	FTDOSDLY 11	/* timeout delay in seconds for concurrent idx/srch */
#ifdef	FTHOS2
#define FTAPIFUN	_pascal _export _loadds
#define FTEXPORT	_export
#define FTLOADDS	_loadds
#else	/*!FTHOS2*/
#ifdef	FTHWIN
#define FTSVSS
#define FTAPIFUN	_pascal _export _loadds
#define FTEXPORT	_export
#define FTLOADDS	_loadds
#else	/*!FTHWIN*/
#define FTAPIFUN	_pascal
#define FTLOADDS
#define FTEXPORT
#endif	/*!FTHWIN*/
#endif	/*!FTHOS2*/
#else	/*!MSDOS*/
#define USHORT	unsigned short
	/* NOTE: FTHUCHAR defined for all remaining machines
	   with BUFSIZ == 512 */
#define FTHUCHAR 1
typedef unsigned char UCHAR;
extern	char	*malloc(), *calloc(), *realloc();
extern	char	*strcpy();

#ifdef	vms
#define register
#define ungetc(x,y) ftvmsungetc(x,y)
#define gmtime localtime
#undef	FTNFILE
#define FTNFILE ftnfile
extern	int	ftnfile;
#define FTHALIGN 1
#define	FTHXLM	1
#define FTVMSLOCK 1
#define FTHFNSC ']'
#define FTHFFNSC	0x5d
#define FTHDYX	 1
#define FTHPNSC ' '
#define FTHXTSC '.'
#define FTHFXTSC	0x2e
#define FTHXEND 1
#define FTHVFORK 1
#define FTHNOVM
#define fcntl(one,two,three)
	/* exit status for vms systems */
#define FTEXOK	1
#define FTEXERR 44
#define FTEXZIP 9
#define FTEXCAN 2096
typedef unsigned long ULONG;

#else	/*!vms*/
#define FTHXTSC '.'
#define FTHFXTSC	0x2e
#define FTHFNSC '/'
#define FTHFFNSC	0x2f
#define FTHPNSC ':'
#define FTHUNIX 1
#ifdef	M68000
	/* 68k Xenix (Spectrix) */
	/* FORWARD also defined */
	/* M_VOID defined here to prevent double typedef in sys/types.h */
#define M_VOID
typedef int void;
#define FTLOCKING
#define FTHALIGN 1
#define FTHTERMIO 1
#define FTHFCNTL 1
#else	/*!M68000*/
	/* misc BUFSIZ == 512 */
#ifndef	_IOSTRG
#if	M_I286 || M_I86
	/* old SCO Xenix was here */
	/* Altos 386 */
#ifdef	ALTOS386
#define FTLOCKING
#else
#define FTLOCKF
#endif
#define FTHTERMIO 1
#define FTHFCNTL 1
#define FTHULIM 1
#define FTHALIGN 1
#define FTHMEMCP 1
#define FTHUTIME 1
#define FTHUSTAT 1
#define	ftuindx	strchr
#define fturindx strrchr
#endif	/*M_I286 || M_I86*/
#endif	/*_IOSTRG*/
#endif	/*!M68000*/
#endif	/*!vms*/
#endif	/*!MSDOS*/
#else	/*BUFSIZ != 512*/
	/* NOTE: unsigned short and unsigned char data types assumed for
	   all machines with BUFSIZ != 512
	   Existence of termio.h and fcntl.h assumed if L_ctermid is defined. */
#define USHORT	unsigned short
#define FTHUCHAR 1
typedef unsigned char UCHAR;

#ifndef osf1		/*91071601*/
#ifndef ultrix		/*91070503*/
#ifndef __hpux		/*91070503*/
#ifndef sun
#ifndef aix             /*90110201*/
extern  char    *malloc(), *calloc(), *realloc();
#endif /*!aix*/
extern	char	*strcpy();
#endif /*sun*/
#endif /*!__hpux*/
#else /*ultrix*/
extern  char	*calloc(); 	/* it is not declared in malloc.h */
#endif /*ultrix*/
#else /* ofs1 */
extern void	*malloc(), *calloc(), *realloc();
#endif /* osf1 */

#define FTHXTSC '.'
#define FTHFXTSC	0x2e
#define FTHALIGN 1
#ifdef	aosvs
#define FTHTERMIO
#define FTHFCNTL
#define FTHMEMCP
#define FTHVFORK
#define	FTHXLM	1
#define FTHXEND
#define FTHFNSC ':'
#define FTHFFNSC	0x3a
#define FTHPNSC ' '
#include <string.h>
	/* bug in free() in stdio.h,string.h */
#undef	free
#define localtime dg_localtime
#define ctime dg_ctime
#else	/*!aosvs*/
#ifdef	mac
#ifndef FTLINT
#define FTLINT
#endif
#include <stdlib.h>		/* testing */
#define UCHAR	unsigned char
#define USHORT	unsigned short
#define	FTHUCHAR	1
#define	FTHFNSC	':'
#define	FTHFFNSC	0x3a
#define	FTHPNSC	','
#define	FTHFCNTL
#define	FTHDFILT
#define	FTHDYX
#define FTHXEND
#define FTHNET	1
#define FTHASH	#
#define FTPRAGMA(x,y) FTHASH pragma x y
#else	/*!mac*/
#define FTHFNSC '/'
#define FTHFFNSC	0x2f
#define FTHPNSC ':'

#if	BUFSIZ != 256 /* Not MVS */
#define FTHUNIX 1
#ifndef	FTHNET
#define FTHNET
#endif
#endif

#ifdef	NLS	/* IBM/RT */
#define FTLOCKF
#endif
#ifdef	__hpux
#include <malloc.h>
#include <string.h>
#define	FTHXLM	1
#define FTLOCKF
#define FTSVSS
#define FTHDYX
#define FTHNFS
#define FTHUTIME
#define FTHVFORK
#define	ftuindx	strchr
#define fturindx strrchr
#ifndef	FTLINT
#ifdef	__STDC__	/* __hp9000s800: cc -Aa -D_HPUX_SOURCE */
#define	FTLINT
#endif
#endif
#ifdef	__hp9000s300
#define FTSIGNAL	signal
#define FTSIGTYPE	void
#endif
#endif	/*__hpux*/
#ifdef	ix370
#define	FTHXLM	1
#define FTLOCKF
#define FTHFHDR
#define FTHDYX
#endif
#ifdef	sun	/* SunOS 4.x */
#include <malloc.h>
#ifndef	SVR4	/* no support for strings.h in SunOS/SVR4 */
#include <strings.h>
#endif	/*!SVR4*/
#include <string.h>
#ifndef __malloc_h	/*91100906 : sun not 4.1.1 */
#ifndef	SVR4	/* not for SunOS/SVR4 */
/*XXX    extern char	*calloc();   XXX*/
#endif	/*!SVR4*/
#endif
#undef	FTNFILE		/* maximum number of open files - defined in */
#define FTNFILE 64	/* sys/param.h as NOFILE - check this if
			   running out of fd's */
#define FTHNFS
#define FTHPOSIX
#define FTHFCNTL
#define FTHVFORK
#define	FTHXLM
#define	FTLFCNTL
#define FTSVSS
#ifdef	SVR4	/* SunOS/SVR4 Test Platform */
#define FTHTERMIO
#define FTHULIM
#define FTHUTIME
#define FTHUSTAT
#define FTHMEMCP
#define	ftuindx		strchr
#define fturindx	strrchr
#define FTSIGTYPE	void
#define FTSIGNAL	sigset
#else	/*!SVR4*/
#define FTSIGNAL	signal
#define FTSIGTYPE	void
#define FTHBSD
#endif	/*SVR4*/
#else	/*!sun*/
#ifdef	L_ctermid
	/* use termio.h instead of sgtty.h */
	/* assume fcntl.h exists */
#define FTHTERMIO 1
#define FTHFCNTL 1
#endif	/*L_ctermid*/
#endif	/*!sun*/

#ifdef osf1
#define FTHXLM		1
#define FTHDYX		1
#define FTHUCHAR	1
#define FTHNFS
#define FTHFNSC		'/'
#define FTHBSD		1
#define FTHUNIX		1
#define FTLOCKF
#define FTHPOSIX
#define FTHFHDR		1
#define FTHVFORK
#define U3B_T		1
#endif	/* osf1 */

#ifdef	CLAN
#ifdef	CLAN7
#define FTCLAN7OR4 1
#endif
#ifdef	CLAN4
#define FTCLAN7OR4 1
#endif
#ifndef	FTCLAN7OR4
#define FTHBHDR 1
#endif
#define FTHULIM 1
#define FTHUSTAT 1
#define FTLOCKF 1
#endif

/* require temporary symbol as MVS does not like #if A | B constructs */
#ifdef	pyr
#define FTSIGNAL	signal
#define FTSIGTYPE	void
#define PYR_T	1
#endif
#ifdef	tx
#define PYR_T	1
#endif
#ifdef	PYR_T /* pyr || tx */
	/* Pyramid  or Tolerant */
#define FTHSYSTH 1
#define	FTHXLM	1
#ifdef	tx
#define sync()
#define FTHBSD	1
#define FTHFCNTL 1
#define FTHVFORK 1
#else	/*pyr*/
#define FTHNFS
#ifndef	P_tmpdir
	/* Pyramid UCB universe with UNIX V locking */
#undef	L_ctermid
#undef	FTHTERMIO
#define FTLOCKF
#define FTHBSD	1
#define FTHFCNTL 1
#define FTHVFORK 1
#else	/*P_tmpdir*/
	/* System V universe */
#define FTHULIM  1
#define FTHUTIME 1
	/* following #ifndef looks like a bug -- krl */
#ifndef	pyr
#define FTHUSTAT 1
#define FTHMEMCP 1
#endif
#define FTLOCKF
#endif	/*P_tmpdir*/
#endif	/*pyr*/
#else	/*not Pyramid or Tolerant*/

#ifdef ultrix                   /*90121001*/
	/* Digital UNIX */
#include <malloc.h>
#include <string.h>
#define	FTHXLM	1
#define FTHSYSTH 1
#define FTSVSS
#ifndef	SYSTEM_FIVE
#if	FTNFILE == 64
	/* ULTRIX BSD mode */
#define FTHNFS
#undef	FTHTERMIO
#define FTLOCKF
#define FTHBSD	1
#define FTHPOSIX
#define FTHFCNTL 1
#define FTHVFORK 1
#define FTSIGNAL	signal
#define FTSIGTYPE	void
#else
	/* UNTESTED ATT System V */
#define FTHMEMCP 1
#define FTHUTIME 1
#define FTHUSTAT 1
#define	ftuindx	strchr
#define fturindx strrchr
#endif	/*FTNFILE != 64*/
#else	/*SYSTEM_FIVE*/
	/* ULTRIX SYSTEM_FIVE mode */
#define FTHMEMCP 1
#define FTHUTIME 1
#define FTHUSTAT 1
#define	ftuindx	strchr
#define fturindx strrchr
#endif	/*SYSTEM_FIVE*/
#else	/*!ultrix*/

	/* UNIX V machine */
#ifdef	uts
#define	FTHXLM	1
#define FTHMEMCP 1              /*90110201*/
#define U3B_T	1
#endif
#ifdef	u3b /* Temp symbol for 3b's as MVS does not like A|B|C construct */
#define FTHMEMCP 1              /*90110201*/
#define U3B_T	1
#endif
#ifdef	u3b2
#define FTHMEMCP 1              /*90110201*/
#define U3B_T	1
#define FTHNOSLT /* there is no 'select' system call */
#endif
#ifdef	u3b5
#define FTHMEMCP 1              /*90110201*/
#define U3B_T	1
#define FTHNOSLT /* there is no 'select' system call */
#endif
#ifndef	sun	/* avoid Sun 386i */
#ifdef	i386
#define FTHNOSLT /* there is no 'select' system call */
#define FTHMEMCP 1              /*90110201*/
#define U3B_T	1
#define FTHNFS	1
#ifdef  SVR4 /* AT&T ix/SVR4 Platform */
#undef  FTCHKSPC
#endif
#endif	/*i386*/
#endif	/*!sun*/
#ifdef	__DGUX__
/* Motorola 88000 - AViiON DG/UX */
#define	FTHXLM	1
#define FTHDYX 1
#define FTHNFS 1
#define FTHVFORK
#define FTHMEMCP 1              /*90110201*/
#define	U3B_T 1
#ifndef	FTLINT
#ifdef	__STDC__
#define	FTLINT
#else
#ifdef	__STDH__	/* DG/UX 4.20 compiler bug */
#define	FTLINT
#endif
#endif	/*!__STDC__*/
#endif
#endif	/*__DGUX__*/
#ifdef  sgi             /*90121701*/
#define FTHVFORK  1
#define FTHDYX  1
#define FTHXLM  1
#define FTHBSD  1
#define FTHPOSIX  1
#define U3B_T   1
#endif  /*sgi*/
#ifdef  aix             /*90110201*/
#define FTHNFS  1
#define FTHDYX  1
#define FTHXLM  1
#define U3B_T   1
#endif  /*aix*/
#if U3B_T /*u3b || u3b2 || u3b5 || i386 || __DGUX__ || aix || sgi  ||  osf1*/
								/*90110201*/
#define FTLOCKF
#define FTHULIM 1
/* #define FTHMEMCP 1 */                /*90110201*/
#define FTHUTIME 1
#define FTHUSTAT 1
#define FTHFHDR 1
#define FTHUCHAR 1
#define FTHALIGN 1
#define	ftuindx	strchr
#define fturindx strrchr
/* you don't want these next two lines if before SYS V 3.1 */
#define FTSIGTYPE	void
#define FTSIGNAL	sigset
#ifdef osf1	/* 91071601 osf1 port, sigset not included in signal.h */
#ifndef osf1sigset
#define osf1sigset
extern void (*sigset( int, void (*)(int))) (int);
#endif
#endif /* osf1 */
#else	/*!U3B_T*/
#ifdef	MOT2000 /* Motorola 2000 */
#define FTHULIM 1
	/* FTNOLOCK defined at the bottom */
#endif	/*MOT2000*/
#ifdef	tx  /* Tolerant */
	/* FTNOLOCK defined at the bottom */
#endif	/*tx*/
#ifdef	mc68k	/* CT M*frame */
	/* CT mightyframe has mc68k mc68k32 mc68020 BUFSIZE 1024 */
#define MC68_T	1 /* Temp symbol as MVS does not like A|B|C|D constructs */
#define types_h /* fake <sys/types.h> because CTIX #defines USHORT */
typedef struct { int r[1]; } *	physadr;
typedef unsigned short	iord_t;
typedef char		swck_t;
typedef unsigned char	use_t;
typedef long		daddr_t;
typedef char *		caddr_t;
typedef unsigned int	uint;
typedef unsigned short	ushort;
typedef unsigned char	unchar;
typedef ushort		ino_t;
typedef short		cnt_t;
typedef long		time_t;
typedef int		label_t[13];	/* regs d2-d7, a2-a7, pc */
typedef short		dev_t;
typedef long		off_t;
typedef long		paddr_t;
typedef long		key_t;
typedef ushort		pgadr_t;
typedef unsigned char	port_t; 	/* local TP/CP port numbers */
typedef unsigned char	mdev_t; 	/* minor device type */
typedef unsigned char	wndw_t; 	/* windows */
#endif	/*mc68k*/
#ifdef	m68	/* plexus 5.2 */
#define MC68_T	1
#endif
#ifdef	m68k	/* altos 3068 */
#define MC68_T	1
#endif
#ifdef	tower	/* NCR tower 2.01.01 */
#define FTHNOSLT
#define FTLOCKF
#define MC68_T	1
#endif

#if MC68_T /* mc68k || m68 || m68k || tower */
#define FTHULIM  1
#define FTHUSTAT 1
#ifdef	mc68k
#define FTLOCKING
#define LK_UNLCK    0
#define LK_RLCK     1
#define LK_NBRLCK   2
#endif
#ifndef	CLAN4
#ifdef	m68k
#define FTLOCKF
#endif
#ifdef	L_ctermid
#define FTHFHDR 1
#endif	/*L_ctermid*/
#ifdef	m68k /* Motorola || altos 3068 */
#ifndef	MOT2000
#define FTLOCKF /* altos only */
#endif
#endif	/*m68k*/
#endif	/*CLAN4*/
#else	/*!mc68k || m68 || m68k || tower*/

#ifdef	sinix
	/* siemens MX2 */
#define FTHFCNTL 1
#define FTHULIM 1
#define FTLOCKING 1
#else	/*!sinix*/
#if BUFSIZ == 256
/* SAS Lattice under MVS */
#define FTHMVS		1
#define	FTHXLM	1
#define FTHNOB		1
#define FTHNOSIG	1
#include <lcstring.h>
#include <lcio.h>
/* No file name or path name separator character support */
#undef	FTHFNSC
#undef	FTHPNSC
#undef	FTHSDM
/* No structure assignments allowed */
#undef	FTHSTASS
/* <stdio.h> standard UNIX symbol */
#define _NFILE		SYS_OPEN
#undef	FTNFILE
#define FTNFILE 	SYS_OPEN
#undef BUFSIZ
#define BUFSIZ		4096
/* Missing errno.h symbols */
#define EACCES		6  /* EFATTR  */
#define EEXIST		6  /* EFATTR  */
#define ENOTDIR 	6  /* EFATTR  */
#define ENFILE		17 /* ELIBERR */
#define EPERM		6  /* EFATTR  */
/* Native Character set is EBCDIC */
#undef	FTHASCII
#define FTHEBCDIC	1
/* Misc issues */
#define _localtm	ftultm
#define ctime		ftuctm
#undef strlen
#undef strcpy
#undef memcpy
#undef memset
#undef memcmp
#else	/*misc BUFSIZ != 512*/
#if M_I86
	/* SCO Xenix 2.2 */
	/* M_XENIX is also defined */
#ifdef	M_I286
/* assumes XENIX-286 will be compiled with -M2 */
#define FTHXEN286
#else	/*!M_I286*/
/* assumes XENIX-386 will be compiled with -M3 */
#define FTHXEN386
#endif	/*!M_I286*/
#ifndef	M_LDATA
	/* FTHSDM no longer supported */
#endif
#define FTLOCKF
#define FTHTERMIO 1
#define FTHFCNTL 1
#define FTHULIM 1
#define FTHALIGN 1
#define FTHMEMCP 1
#define FTHUTIME 1
#define FTHUSTAT 1
#define	ftuindx	strchr
#define fturindx strrchr
#else	/*!Xenix 286 2.2*/
/* some may require: typedef	int void; */
#endif	/*M_I86*/
#endif	/*BUFSIZ != 256*/
#endif	/*!sinix*/
#endif	/*!mc68k || m68 || m68k || tower*/
#endif	/*!u3b*/
#endif	/*!ultrix*/
#endif	/*!pyr*/
#endif	/*!mac*/
#endif	/*!aosvs*/
#endif	/*BUFSIZ != 512*/

	/* This stuff only valid for Microsoft C */
#ifndef	FTHMSDOS
#define FTBASED(x)
#define FTOFF(x)	x
#define PASCAL
#define CDECL
#define NEAR
#define FAR
#endif

#ifndef	FTDLSTAT
#define FTDLSTAT
#endif

#ifdef	FTHDEVEL
	/* enable lock tracking */
/* #define FTLTRACK */
#endif

	/* raw I/O functions */
#ifdef	FTLTRACK
	/* define arbitrary limit on number of locks in a single file */
#define FTLKMAX 100
#endif	/*FTLTRACK*/

	/* miscellaneous macros for 4.x compatibility */
#ifdef	FTHUNIX
#define	ftffdtor(rfd)	ftrfdtor(ftapih,rfd)
#endif

#ifndef	vms
	/* exit status for non-vms systems */
#define FTEXOK	0
#define FTEXERR 1
#define FTEXZIP 2
#define FTEXCAN 3
#ifndef	FTHMSDOS
extern	int	errno;	/* not all systems have this in <errno.h> */
#endif
#endif	/*!vms*/

	/* FTDEBUG macros */
#define FTDBGON(a)
#define FTDBGOFF(a)
#define FTDBGWHO(a)
#ifdef	FTDEBUG
#ifndef	FTDLLIB
#define register
#define FTDBG(a)	ftdbg a
#endif
#endif	/*FTDEBUG*/

#ifndef	FTDBG
#define FTDBG(a)
#ifdef	FTHNOREG
#ifndef	register
#define register
#endif
#endif
#endif	/*!FTDBG*/

#ifdef	FTTEST
#define FTVBG(a)	if (ftvbose) ftdbg a
#else
#define FTVBG(a)
#endif

	/* FTWHO typedef for declaring numeric who variables */
typedef char NEAR * FTWHO;

	/* entry and exit macros for instrumented Ful/Text and debugging */
#ifdef	FTHINS
#define ftentry()	ftzentr(who)
#define ftonret()	ftzexit(who)
#define ftretvoid	{ftonret(); return;}
#define ftreturn(v)	return(ftonret(),(v))
#else	/*!FTHINS*/
#ifdef	FTDEBUG
#ifndef	FTDLLIB
#define ftentry()	ftuprntf(stderr,"%w: entry\n",who)
#define ftonret()	ftuprntf(stderr,"%w: exit\n",who)
#define ftretvoid	{ftonret(); return;}
#define ftreturn(v)	return(ftonret(),(v))
#endif
#endif	/*FTDEBUG*/
#ifndef	ftentry
#define ftentry()
#define ftretvoid	return
#define ftreturn(v)	return(v)
#endif	/*!ftentry*/
#endif	/*!FTHINS*/

	/* unsigned char to int conversion macros: */
#ifdef	FTHMSDOS
#define ftuctoi(c)	((int)(c))
#define ftpuctoi(p)	((int)*(p))
#define ftppucti(p)	((int)*(p)++)
#else	/*!FTHMSDOS*/
#ifdef	FTHUCHAR
#define ftuctoi(c)	((int)(UCHAR)(c))
#define ftpuctoi(p)	((int)(*(UCHAR *)(p)))
#define ftppucti(p)	((int)(*(UCHAR *)(p)++))
#else	/*!FTHUCHAR*/
#define ftuctoi(c)	((int)(c)&0xff)
#define ftpuctoi(p)	((int)(*(p))&0xff)
#define ftppucti(p)	((int)(*(p)++)&0xff)
#endif	/*!FTHUCHAR*/
#endif	/*!FTHMSDOS*/

#ifndef	FTFLOCK
#ifndef	FTLOCKF
#ifndef	FTLOCKING
#ifndef	FTVMSLOCK
#ifndef	FTLFCNTL
#define FTNOLOCK 1
#ifdef	FTHNFS
#undef	FTHNFS		/* don't set this unless some locking available */
#endif
#endif
#endif
#endif
#endif
#endif

#ifdef	FTHUNIX
#ifndef	FTHDYX
#define FTHDYX	1
#endif
#ifndef	FTHNET
#define FTHNET	1
#endif
#endif

/* memory copy and structure assignment macros */
#ifdef	FTHMEMCP

#ifdef	FTHMSDOS
void * CDECL memcpy(void *, CONST void *, SIZE_T);
void _far * FTAPIFUN ftucfblk(void _far *, CONST void _far *, SIZE_T);
#define FTCOPFBLK(d,s,l) ftucfblk((d),(s),(l))
#endif

#define FTCOPBLK(d,s,l)  memcpy((d),(s),(l))
#ifndef	FTCOPFBLK
#define FTCOPFBLK(d,s,l) memcpy((d),(s),(l))
#endif	/*!FTCOPFBLK*/

#else	/*!FTHMEMCP*/
extern	char	*ftucblk();
#define FTCOPBLK(d,s,l)  ftucblk((d),(s),(l))
#define FTCOPFBLK(d,s,l) ftucblk((d),(s),(l))
#endif	/*!FTHMEMCP*/

#ifdef	FTHSTASS
#define FTSTASS(d,s,l) d = s
#else
#define FTSTASS(d,s,l) FTCOPBLK((char *) &d, (char *) &s, (l));
#endif

/* Assign the default filter list for reading the stop/msg files */
#ifdef	FTHMVS
#define FTHDFILT    "ntim:s"
#else	/*!FTHMVS*/
#define FTHDFILT    "s"
#endif

#ifdef	FTHWIN		/* Microsoft Windows */
#define FTHNOVM 	/* don't allow void main() definition */
/* define scheduler routine */
void ftwsked(void);
#define FTWSKED ftwsked();
/* redefine standard streams */
#ifndef	FTNOD
extern FILE	*ftstderr;
#undef		stderr
#undef		stdout
#define 	stderr	ftstderr
#define 	stdout	stderr
#endif	/*!FTNOD*/
#else	/*!FTHWIN -- ALL non-Windows systems*/
#define FTWSKED 	/* do nothing under non-Windows */
#endif	/*!FTHWIN*/

#ifdef	FTHDYX		/* Ensure B-tree code is compiled if FTHDYX */
#ifndef	FTHBTREE
#define FTHBTREE	1
#endif
#endif

/* define disk space checking for System V, MSDOS */
#ifdef	FTHMSDOS
#define FTCHKSPC	1
#else
#ifdef	FTHUSTAT
#ifndef	SVR4
#define FTCHKSPC	1
#endif
#endif	/*FTHUSTAT*/
#endif	/*FTHMSDOS*/

	/* ftusleep is a no-op for Windows and mac */
#ifdef	FTHWIN
#define	ftusleep(x)
#endif
#ifdef	mac
#define	ftusleep(x)
#endif

	/* Size calculation giving fully aligned size */
#define FTASIZE(x)  (sizeof(x)-1 + sizeof(long) - (sizeof(x)-1)%sizeof(long))

	/* handles */
#ifdef	mac		/*91011401*/
typedef	char **	FTHNDL;	/* Same as type Handle from <Types.h> */
#else
#ifdef	FTHDEVEL
typedef USHORT	FTHNDL;
#else
#ifdef	FTHWIN
typedef USHORT	FTHNDL;
#else
#ifdef	FTHSDM
typedef USHORT	FTHNDL;
#else
typedef int *	FTHNDL;
#endif
#endif	/*!FTHWIN*/
#endif	/*!FTHDEVEL*/
#endif	/*!mac*/

#ifndef	FTHMSDOS		/* no distinction between NEAR and FAR...
			   e.g. UNIX, or XENIX LDM */
#define CDECL
#ifndef	PASCAL
#define PASCAL
#endif
	/* far and near pool functions are identical	 */
#define ftpfpstk	ftpnpstk
#define ftpftstk	ftpntstk
#define ftpffstk	ftpnfstk
#define ftpfplay	ftpnplay
#define ftpftlay	ftpntlay
#define ftpfflay	ftpnflay

#endif	/*!FTHMSDOS*/

/* define interface modules to malloc(), calloc(), realloc() and free() */
	/* FTMT enables memory tracing */
#ifdef	FTTEST
#define	FTMT
#endif

#ifndef	mac		/*91011401*/
#ifdef	FTMT
#ifdef	FTLINT
extern int	ftuqhndl(FTWHO);
extern void	ftuminit(void);
extern char	*ftumalloc(FTWHO, FTWHO, unsigned);
extern char	*ftucalloc(FTWHO, FTWHO, unsigned, unsigned);
extern char	*fturealc(FTWHO, FTWHO, char *, unsigned);
extern void	ftufree(FTWHO, FTWHO, char *);
extern void	ftucache(FTWHO, FTWHO, char *);
extern void	ftureuse(FTWHO, FTWHO, char *);
#else	/*!FTLINT*/
extern	int	ftuqhndl();
extern void	ftuminit();
extern	char	*ftumalloc(),*ftucalloc(),*fturealc();
extern void	ftufree();
extern void	ftucache(),ftureuse();
#endif	/*!FTLINT*/
#ifdef	FTHMSDOS
extern char	NEAR *ftummallc(FTWHO, FTWHO, unsigned);
extern void	ftunfree(FTWHO, FTWHO, char NEAR *);
#endif

#define ftmalloc(who,what,size)		ftumalloc(who,what,size)
#define ftcalloc(who,what,n,size)	ftucalloc(who,what,n,size)
#define ftrealc(who,what,p,size)	fturealc(who,what,p,size)
#define ftcache(who,what,p)		ftucache(who,what,p)
#define ftreuse(who,what,p)		ftureuse(who,what,p)
#define ftfree(who,what,p)		ftufree(who,what,p)
#ifdef	FTHWIN
#define	ftmmallc(who,what,size)		ftummallc(who,what,size)
	/* allocate explicitly near uninitialized movable memory */
#define ftmmnalc(who,what,size)		ftummallc(who,what,size)
#define ftmnfre(who,what,p)		ftunfree(who,what,p)
#define	ftmnfix(who,what,h)		ftumnfix(h)
#define	ftmnbrk(who,what,p,ph)		ftumnbrk(p,ph)
#else	/*!FTHWIN*/
#define ftmmallc(who,what,size)		ftumalloc(who,what,size)
#ifdef	FTHMSDOS
	/* allocate explicitly near uninitialized movable memory */
#define ftmmnalc(who,what,size)		ftummallc(who,what,size)
#define ftmnfre(who,what,p)		ftunfree(who,what,p)
#else
#define ftmmnalc(who,what,size)		ftumalloc(who,what,size)
#define ftmnfre(who,what,p)		ftufree(who,what,p)
#endif
#endif	/*!FTHWIN*/

#ifdef	FTHDEVEL
	/* ftufix() ftubrk() prototypes moved to fthandle.h after FTHNDL */
#define ftfix(who,what,h)		ftufix(who,what,h)
#define ftbrk(who,what,p,ph)		ftubrk(who,what,p,ph)
#ifndef	FTHWIN
#ifndef	M_I86LM       /* !Microsoft large model */
#define	ftmnfix(who,what,h)		ftufix(who,what,h)
#define	ftmnbrk(who,what,p,ph)		ftubrk(who,what,p,ph)
#endif
#endif
	/* the following macros prevent compilation if an attempt is
	   made to use these functions directly without #undef'ing first */
#define malloc(n)		++error++
#define calloc(n,e)		++error++
#define realloc(n)		++error++
#define free(p) 		++error++
#endif	/*FTHDEVEL*/

#else	/*!FTMT*/

#ifdef	FTHWIN
#ifndef	M_I86LM       /* !Microsoft large model */
#define ftmalloc(who,what,size) 	ftumnalc((size),-1)
#define ftcalloc(who,what,n,size)	ftumnalc((size)*(n),0)
#define ftmmallc(who,what,size)		ftummnalc((size),-1)
#define ftrealc(who,what,p,size)	ftumnrlc(ftapih,p,size)
#define ftfree(who,what,p)		ftumnfre(p)
#define ftfix(who,what,h)		ftumnfix(h)
#define ftbrk(who,what,p,ph)		ftumnbrk(p,ph)
#else	/*M_I86LM*/
#define ftmalloc(who,what,size)         ftumfalx((size),FTGMFIXED)
#define ftcalloc(who,what,n,size)       ftumfalx((size)*(n),FTGMFIXED|FTGMZINIT)
#define ftmmallc(who,what,size)         ftumfalx(size,0)
#define ftrealc(who,what,p,size)        ftumfcsx(p,size,0)
#define ftfree(who,what,p)              ftumffre(p)
#define ftfix(who,what,h)               ftumffix(h)
#define ftbrk(who,what,p,ph)            ftumfbrk(p,ph)
#endif	/*M_I86LM*/
	/* allocate explicitly near uninitialized movable memory */
#define ftmmnalc(who,what,size)		ftummnalc((size),-1)
#define ftmnfre(who,what,p)		ftumnfre(p)
#define	ftmnfix(who,what,h)		ftumnfix(h)
#define	ftmnbrk(who,what,p,ph)		ftumnbrk(p,ph)
#else	/*!FTHWIN*/
#define ftmalloc(who,what,size)		malloc(size)
#define ftcalloc(who,what,n,size)	calloc(n,size)
#define ftmmallc(who,what,size)		malloc(size)
#define ftrealc(who,what,p,size)	realloc(p,size)
#define ftfree(who,what,p)		free(p)
#ifdef	FTHMSDOS
	/* allocate explicitly near uninitialized movable memory */
#define ftmmnalc(who,what,size)		_nmalloc(size)
#define ftmnfre(who,what,p)		_nfree(p)
#else
#define ftmmnalc(who,what,size)		malloc(size)
#define ftmnfre(who,what,p)		free(p)
#endif
#endif	/*!FTHWIN*/
#define ftcache(who,what,p)
#define ftreuse(who,what,p)
#endif	/*!FTMT*/
#else	/*mac*/
#define ftmalloc(who,what,size) 	ftumnalc((size),-1)
#define ftcalloc(who,what,n,size)	ftumnalc((size)*(n),0)
#define ftmmallc(who,what,size)		ftumnalc((size),-1)
#define ftmmnalc(who,what,size)		ftumnalc((size),-1)
#define ftrealc(who,what,p,size)	ftumnrlc(ftapih,p,size)
#define ftfree(who,what,p)		ftumnfre(p)
#define ftmnfre(who,what,p)		ftumnfre(p)
#define ftfix(who,what,h)		ftumnfix(h)
#define ftmnfix(who,what,h)		ftumnfix(h)
#define ftbrk(who,what,p,ph)		ftumnbrk(p,ph)
#define ftmnbrk(who,what,p,ph)		ftumnbrk(p,ph)
#define ftcache(who,what,p)
#define ftreuse(who,what,p)
extern char *	ftumnalc(unsigned, int);
extern void	ftumnfre(char *);
extern char *	ftumnfix(FTHNDL);
extern int	ftumnbrk(char *,FTHNDL *);
/* ftumnrlc() in fthandle.h */
#ifdef	FTHDEVEL
	/* the following macros prevent compilation if an attempt is
	   made to use these functions directly without #undef'ing first */
#define malloc(n)		++error++
#define calloc(n,e)		++error++
#define realloc(n)		++error++
#define free(p) 		++error++
#endif	/*FTHDEVEL*/
#endif	/*mac*/

#ifndef	ftfix
#define ftfix(who,what,h)		((char *)h)
#define ftbrk(who,what,p,ph)		(ph ? (*ph = (FTHNDL)p) : 0,0)
#endif
#ifndef	ftmnfix
#ifdef	FTHMSDOS
#define	ftmnfix(who,what,h)	((char NEAR *)(unsigned)(long)h)
#define	ftmnbrk(who,what,p,ph)	(ph ? (*ph = (FTHNDL)(long)(unsigned)p) : 0,0)
#else
#define	ftmnfix(who,what,h)		((char NEAR *)h)
#define	ftmnbrk(who,what,p,ph)		(ph ? (*ph = (FTHNDL)p) : 0,0)
#endif
#endif

#ifdef	FTHWIN
	/* these prototypes needed FTMT or not */
extern char NEAR * FTEXPORT PASCAL	ftumnalc(unsigned, int);
extern char NEAR * FTEXPORT PASCAL	ftummnalc(unsigned, int);
extern void FTEXPORT PASCAL		ftumnfre(char NEAR *);
extern char NEAR * FTEXPORT PASCAL	ftumnfix(FTHNDL);
extern int FTEXPORT PASCAL		ftumnbrk(char NEAR *,FTHNDL FAR *);
/* ftumnrlc() in fthandle.h */
#ifdef	 M_I86LM
extern char FAR * PASCAL	ftumfalx(unsigned, unsigned);
extern char FAR * PASCAL	ftumfcsx(char FAR *, unsigned, unsigned);
extern void FTAPIFUN		ftumffre(char FAR *);
extern char FAR * FTAPIFUN	ftumffix(FTHNDL);
extern int FTAPIFUN		ftumfbrk(char FAR *, FTHNDL FAR *);
#endif	/*M_I86LM*/
#endif	/*FTHWIN*/

	/* define interface functions to unlink(), ftutfnam() */
#ifdef	FTHDEVEL
#define FTHTFLOG 1
extern	int	ftuunlnk(), ftutflog();
extern	void	ftutfxtr();
#define ftunlink	ftuunlnk
#define ftutfcr 	ftutflog
#define ftutfchk	ftutfxtr
#else	/*!FTHDEVEL*/
#define ftunlink(who,what,fn)	ftrfdel(ftapih,fn)
#define ftutfcr(who,what,d,f)	ftutfnam(d,f)
#define ftutfchk(who)
#endif	/*!FTHDEVEL*/

	/* define FAR version of ftuprntf() */
#ifdef	FTHDOSSDM
extern CDECL ftufprtf(FILE *, char FAR *, ...);
#ifdef	 FTDEBUG
#define FTFDBG(a)	ftfdbg a
#else
#define FTFDBG(a)
#endif	/*!FTDEBUG*/
#else	/*!FTHDOSSDM*/
#define	ftufprtf	ftuprntf
#define FTFDBG(a)	FTDBG(a)
#endif	/*!FTHDOSSDM*/

	/* FTPL is used in ft?funcs.h files to drop prototypes where */
	/* they are not supported */
#ifdef	FTLINT
	/* prototype to be used only if FTLINT is defined */
#define FTPL(x)	x
	/* prototype required in MSDOS */
#define FTPD(x)	x
#else	/*!FTLINT*/
#ifdef	FTHMSDOS
#define FTPD(x)	x
#else
#define FTPD(x) ()
#endif
#define FTPL(x) ()
#endif	/*!FTLINT*/

#ifdef	FTHNOVM
#define	FTMVOID	int
#else
#define	FTMVOID	void CDECL
#endif

	/* for environments where strchr(), strrchr() not supported */
#ifndef	ftuindx
#ifdef	FTHBSD
#define ftuindx index
#define fturindx rindex
#endif
#endif

#ifndef	FTPRAGMA
#define FTPRAGMA(x,y)
#endif

#else	/*FTHVMCMS*/

/*	IBM C compiler	     */
#include <stdio.h>

#define NULL		0

#define FTHEBCDIC	1
#define FTHLEMMA	1
#define FTHSSPA		1

     /* exit status */
#define FTEXOK		0
#define FTEXERR		1
#define FTEXZIP		2
#define FTEXCAN		3

/* Missing errno.h symbols */
#define EACCES		6  /* EFATTR  */

typedef int *		FTHNDL;
typedef char *		FTWHO;

#define USHORT		unsigned short
#define FTHUCHAR	1
typedef unsigned char	UCHAR;
extern	char		*strcpy();
extern	int		errno;

#define FTHFGSC		'#'

#define FTHXTSC		'.'
#define FTHFXTSC	0x2e
#define FTHFNSC		'.'
#define FTHFFNSC	0x2e
#define FTHPNSC		'.'

#define FTHMEMCP
#define FTCOPBLK(d,s,l) memcpy((d),(s),(l))
#define FTCOPFBLK(d,s,l) memcpy((d),(s),(l))
#define FTSTASS(d,s,l)	FTCOPBLK((char *) &d, (char *) &s, (l))

	/* Size calculation giving fully aligned size */
#define FTASIZE(x)  (sizeof(x)-1 + sizeof(long) - (sizeof(x)-1)%sizeof(long))
#define FTPD(x)		()
#define FTPL(x)		()

#define ftfix(who,what,h)		((char *)h)
#define ftbrk(who,what,p,ph)		(ph ? (*ph = (FTHNDL)p) : 0,0)

#define _NFILE		30
#define FTNFILE		_NFILE
#define FTBIGCAT
#define FTHDYX
#define FTHBTREE
#define FTHNOSIG
#define FTLOCKF
#define FTHXLM
#define FTUSTAT

#define PASCAL
#define NEAR
#define FAR
#define FTMVOID		void
#define FTDLSTAT	static

/* deafult filter list + I/O layer parm for reading the stop/msg files */
#define FTHDFILT	"ntim:s(fty=txt)"

#define ftubrchk()	0
#define ftusleep(x)

/* Change general I/O layer routines to ftvmXXXX() */
#define unlink(x)	ftvmunlk(x)

/* 'parm' for the VM/CMS I/O layer */
#define FTYBIN		"(fty=bin)"
#define FTYTXT		"(fty=txt)"

#define ftuctoi(c)	((int)(UCHAR)(c))
#define ftpuctoi(p)	((int)(*(UCHAR *)(p)))
#define ftppucti(p)	((int)(*(p)++)&0xff)

extern char	*ftumalloc(), *ftucalloc(), *fturealc();
#define ftmalloc(who,what,size)		ftumalloc(who,what,size)
#define ftmmnalc(who,what,size)		ftumalloc(who,what,size)
#define ftcalloc(who,what,n,size)	ftucalloc(who,what,n,size)
#define ftrealc(who,what,p,size)	fturealc(who,what,p,size)
#define ftfree(who,what,p)		ftufree(who,what,p)

#ifdef	FTDEBUG
#define ftentry()	ftuprntf(stderr,"%w: entry\n",who)
#define ftonret()	ftuprntf(stderr,"%w: exit\n",who)
#define ftretvoid	{ftonret(); return;}
#define ftreturn(v)	return(ftonret(),(v))
#else	/*!FTDEBUG*/
#define ftentry()
#define ftretvoid	return
#define ftreturn(v)	return(v)
#endif	/*!FTDEBUG*/

	/* FTDEBUG macros */
#define FTDBGON(a)
#define FTDBGOFF(a)
#define FTDBGWHO(a)
#ifdef	FTDEBUG
#define register
#define FTDBG(a)	ftdbg a
#define FTFDBG(a)	ftfdbg a
#else
#define FTDBG(a)
#define FTFDBG(a)
#endif

#ifdef	FTTEST
#define FTVBG(a)	if (ftvbose) ftdbg a
#else
#define FTVBG(a)
#endif

#endif	/*FTHVMCMS*/

/* The following two definitions were moved from ftgmem.h (needed by WIN LDM) */
#define FTGMFIXED       2       /* memory block is not moveable */
#define FTGMZINIT       4       /* memory block is be zeroed at allocation */

#ifdef	__STDC__
	/* compiler is ANSI-compliant */
#define FTHANSIC
#endif
#ifdef	FTHMSDOS
#ifndef	FTHWIN
	/* stdarg.h doesn't work in a Windows DLL */
#define FTHANSIC
#endif
#endif

	/* assume const keyword available if FTLINT set (ANSI C prototypes) */
#ifndef	CONST
#ifdef	FTLINT
#define CONST	const
#else
#define CONST
#endif
#endif	/*!CONST*/

	/* FTAPIFUN is only defined for OS/2, Windows */
#ifndef	FTAPIFUN
#define FTAPIFUN	PASCAL
#define FTLOADDS
#define FTEXPORT
#endif

/* unconditionally defined for now */
#define	FTHPROTO

extern char FAR *ftugenv FTPL((const char *));

/* code segmentation */
#ifdef	ftkernel		/*91021401*/
#ifdef	mac
#include "ftsegment.h"
#endif
#endif

#endif	/*fthio_h*/
