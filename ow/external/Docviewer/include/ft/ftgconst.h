/*static char *sccsid="@(#) ftgconst.h (1.3)  14:21:05 07/08/97";

  ftgconst.h -- general constants for API functions and their callers

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------
  | crs 5.1C 92/02/27 | FTGREL -> C201
  | ah  5.1B 92/01/06 | FTGREL -> 5.1C2
  | ah  5.1B 92/01/03 | FTGREL -> 5.1C1
  | ah  5.1B 91/12/30 | FTGREL -> 5.1B11
  | ah  5.1B 91/12/17 | FTGREL -> 5.1B10
  | ah  5.1B 91/11/18 | FTGREL -> 5.1B9
  | ah  5.1B 91/11/07 | FTGREL -> 5.1B8
  | ah  5.0F 91/10/30 | 91052401: FTBSHARE
  | ah  5.1B 91/10/16 | FTGREL -> 5.1B7
  | ah  5.1B 91/09/30 | FTGREL -> 5.1B6
  | rdc 5.1B 91/09/11 | Added FTFXBELE, FTFXEELE
  | ah  5.1B 91/09/06 | FTGREL -> 5.1B5
  | wtr 5.1B 91/09/06 | Add FTGSEQ
  | rdc 5.1B 91/09/02 | FTFXBM, FTFUBM, FTUCRCOV
  | wtr 5.1B 91/08/30 | change SQL datatypes to kernel data types
  | wtr 5.1B 91/08/29 | add SQL data types
  | swr 5.1B 91/08/27 | 90111601: Mac: add defn of native char set
  | ah  5.1B 91/08/21 | Add FTLOCKR to FTCLOCKM
  | PW  5.1B 91/07/11 | FTCDDTD, FTCULMT
  | PW  5.1B 91/06/27 | FTCIVAL 
  | wtr 5.1B 91/07/08 | FTGREL -> 5.1B4
  | wtr 5.1B 91/06/25 | FTGREL -> 5.1B3
  | StM 5.1B 91/06/21 | add FTNMXFILT, resize FTGMXFILT
  | hjb 5.1B 91/06/19 | FTALSVLP
  | PW  5.1B 91/06/10 | FTDTDFID
  | PW  5.1B 91/06/07 | FTCDTD
  | rtk 5.1B 91/06/10 | FTGREL -> 5.1B2
  | ded 5.1B 91/06/06 | Defined FTCFFBFSZ, FTCDMBFSZ and FTCNFFLDS
  | rtk 5.1B 91/05/29 | FTGREL -> 5.1B1
  | rtk 5.1A 91/05/22 | FTGREL -> 5.1A4
  | wtr 5.1A 91/05/14 | Get rid of nested comments
  | rtk 5.0E 91/05/08 | add FTAXLIT (90032203)
  | rtk 5.1A 91/04/29 | move FTFOFLG to ftfconst.h
  | bf  5.1A 91/04/18 | 91022503: reinstate FTSRBKMX
  | cps 5.1A 91/04/10 | Move FTSNOSR definition outside of FTSVSS ifdef
  | cps 5.1A 91/04/08 | Add FTSNOSR
  | PW  5.1A 90/11/28 | FTNBINS
  | PW  5.1A 90/11/28 | FTCNRD
  | hjb	5.1W 91/02/22 | 91013001: remove declaration of extern ftuargs()
  | swr 5.1A 91/02/21 | 90121001: DECstation 5000 port (vax -> ultrix)  
  | cps 5.1A 91/02/11 | Add FTSQCID, FTSQNAME, FTSOOBJ (ifdef'd for FTSVSS)
  | ah  5.0W 91/02/08 | FTGREL -> 5.1A2
  | bf  5.0W 90/11/06 | msdos: change FTUSIGNM to 2
  | rdc 5.0D 90/10/24 | 90102401: increase FTGWHOSIZ from 16 to 32
  | rtk 5.0D 90/10/01 | FTQMAXFR is now 20 in all environments
  | bf  5.0D 90/09/20 | drop FTSRBKMX
  | PW  5.0D 90/09/18 | define FTCSTATIC
  | rtk 5.0D 90/09/12 | define FTSQNULL
  | hjb	5.0D 90/08/16 | FTFXOFLO (90081402)
  | DAT 5.0D 90/08/11 | SM: VM port
  | bf  5.0D 90/08/10 | add FTGBCSIZ,FTGBISIZ
  | bf  5.0D 90/08/02 | add FTALNCON
  | ah  5.0D 90/06/28 | remove FTBIGCAT #ifdefs
  | KRL 5.0D 90/06/21 | istates to ftitypes.h
  | rtk 5.0C 90/06/11 | change FTQMAXSTW to 1000
  | KRL 5.0C 90/06/08 | FTGBBSLIM
  | rtk 5.0C 90/06/07 | avoid nested comments
  | KRL 5.0C 90/05/21 | FTIIFAIL2
  | rtk 5.0C 90/05/09 | add FTUFCREAT
  | KRL 5.0C 90/05/02 | __hpux
  | ah  5.0C 90/05/02 | Added FTGCRTP
  | bf  5.0C 90/04/23 | move FTNSCNOD from ftnconst.h; add FTNLOCAL
  | bf  5.0C 90/04/11 | FTUCLLOC
  | bf  5.0C 90/04/09 | drop FTBDEV (89011803)
  | KRL 5.0C 90/03/27 | FTGNDAT* FTGNIND*
  | rtk 5.0C 90/03/24 | FTVDWGHT, FTVRNKWT, FTVRNKDT
  | PW  5.0C 90/03/14 | FTXMSCND
  | DG	5.0C 90/03/09 | FTCEFATAL, FTCENETIO
  | DG	5.0C 90/03/06 | FTSHCFxxx
  | PW  5.0B 90/01/15 | FTQMAXSTW
  | DG	5.0B 90/01/12 | FTGSWAPOUT
  | bf  5.0B 90/01/11 | FTBREMOTE
  | ah  5.0A 89/12/22 | FTCOVWRT
  | bf  5.0A 89/12/14 | fix bit values for FTASV constants
  | bf  5.0A 89/12/11 | FTUSIGNM
  | ah  5.0A 89/11/27 | FTCFIRST, FTCNEXT, FTCEND, FTCPREV, FTCLOCKM
  | rtk 5.0A 89/11/25 | FTSBEKEEP
  | bf  5.0  89/11/09 | istate constants; drop FTBIXOK; long FTB flags
  | rl  5.0  89/11/09 | FTUFPNAM
  | PW  5.0  89/11/09 | FTGSMXWT
  | ah  5.0  89/11/06 | FTCBACKW
  | rl  5.0  89/11/06 | FTBNCOMP
  | KRL 5.0  89/11/04 | FTXDCTFLG
  | bf	5.0  89/10/26 | FTBNOIWRD,FTBNOIBLD
  | AH  5.0  89/10/23 | FTCINIOK
  | KRL 5.0  89/10/06 | OS/2 1.2 HPFS: FTGPATHSZ 261
  | KRL 5.0  89/09/20 | FTFIDDLEN
  | bf  5.0A 89/09/18 | FTCNONE & FTCWSUBD
  | ah  5.0A 89/09/08 | FTCVNOUL
  | bf  4.6C 89/08/30 | set FTGSRTMEM,FTMRGMEM to 65500 for LDM DOS (OS/2)
  | AH	5.0A 89/08/30 | Moved FTCVREL & FTCVSFRE here from ftcvtyp.h
  | AH	5.0A 89/08/29 | Incremented FTCVTSZ
  | rl  5.0  89/08/23 | FTASVMSG
  | bf  4.6C 89/08/25 | FTBUNLOCK (#89082302)
  | rl  5.0  89/08/22 | FTGFNCTR for central minifile
  | ah  5.0  89/08/21 | FTCGFID for ftcvstat()
  | bf  5.0  89/08/14 | FTUCPRSV
  | DG	5.0  89/08/08 | FTALxxxx
  | RL  5.0A 89/08/04 | FTANLONG for ftapiini()
  | AH  5.0A 89/08/03 | FTCSTRM for ftcopen().
  | bf  4.6C 89/08/03 | FTCSDPREL
  | KRL 5.0  89/08/01 | increase FTDELMAX for hpux
  | AH	5.0A 89/08/01 | FTCVTSZ and FTCVSD - moved from ftcvtyp.h
  | DG	5.0  89/07/26 | FTGFUCO
  | DG	5.0  89/07/11 | FTASVxxxx; FTGFUPO
  | SM  5.0  89/06/26 | FTCSDNOSUB
  | pbc 5.0  89/06/01 | FTGVER, FTGREL
  |-------------------|---------------------------------

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */

/*	NOTE: Symbols which are preceded by the comment PRIVATE
	      are not for use by callers of the Ful/Text API */

#ifndef ftgcon_h
#define ftgcon_h	1
#ifndef fthio_h
#include "fthio.h"
#endif

	/* Ful/Text Version 5.1 */
#define FTGVER		51
#define FTGREL		"C201"
#define FTGSEQ		1

/* Maximum length of collection names */
#ifdef FTHMSDOS
#define FTGMCOL 	8
#else
#ifdef FTHMVS
#define FTGMCOL 	5
#else
#define FTGMCOL 	10
#endif
#endif

/* maximum possible value of FTGMCOL for any system */
#ifdef	FTHNET
#define FTGMMCOL	10
#else
#define FTGMMCOL	FTGMCOL
#endif

#ifdef	FTHMSDOS
#ifndef	FTHOS2
	/* maximum length (+1) of a path name (see MSC <stdlib.h>) */
#define FTGPATHSZ	80
	/* maximum size of command line */
#define FTGCMDSIZ	128
#else	/* FTHOS2 */
#define FTGPATHSZ	261
#define FTGCMDSIZ	512
#endif	/* FTHOS2 */
#else	/*!FTHMSDOS*/
	/* maximum length (+1) of a path name */
#define FTGPATHSZ	256
#ifdef  FTHUNIX
	/* maximum size of command line */
#define FTGCMDSIZ	512
#else
#define FTGCMDSIZ	1024
#endif  /*!FTHUNIX*/
#endif	/*!FTHMSDOS*/

	/* maximum size of a path name in a network environment */
#ifdef	FTHNET
#define FTNPATHSZ	261
#else
#define FTNPATHSZ	FTGPATHSZ
#endif

	/* maximum length (+1) of a device name */
#define FTGDEVSZ	4

	/* maximum length (+1) of a node name */
#define FTGNODESZ	80

	/* PRIVATE maximum length (+1) of a topic field line */
#define FTGTPLSIZ	FTGPATHSZ+135

	/* maximum length (+1) of a word in any environment */
#define FTGWORDMSZ	251
	/* maximum length (+1) of a word for indexing or for query analysis */
#ifdef	FTHMSDOS
	/* conserve memory in small environments */
#define FTGWORDSZ	81
#else
#define FTGWORDSZ	FTGWORDMSZ
#endif	/*FTHMSDOS*/
	/* maximum number of concurrently indexed zones*/
#define FTIMAXCZ	(((FTGWORDSZ-1)/2)-1)

	/* maximum size of a configuration file buffer */
#ifdef FTHMSDOS
#define FTGFIGSIZ	1024
#else	/* !FTHMSDOS */
#define FTGFIGSIZ	1536
#endif

	/* maximum length (+1) of a collection name */
#define FTGCNAMSZ	61

	/* maximum length (+1) of a username and password */
#define FTGMXUNAM	16
#define FTGMXPASS	32

	/* maximum size of concordance	*/
#define FTGTXTINF	2L<<22

	/* lower bound for ftindex -b and -m parms */
#define FTGILBND	2048

	/* default sort/merge memory area */
#ifdef FTHSDM
#ifdef FTHMSDOS
#define FTGSRTMEM	16384L
#define FTGMRGMEM	40000L
#else	/*!FTHMSDOS*/
#define FTGSRTMEM	40000L
#define FTGMRGMEM	40000L
#endif
#else	/*!FTHSDM*/
#ifdef	FTHXEN286
/* the following value works for XENIX-286 but */
/* disagrees with malloc() documentation       */
#define FTGSRTMEM	65500L
#define FTGMRGMEM	65500L
#else	/*!FTHXEN286*/
#ifdef	FTHMSDOS
#define FTGSRTMEM	65500L
#define FTGMRGMEM	65500L
#else	/*!FTHMSDOS*/
#define FTGSRTMEM	206000L
#define FTGMRGMEM	40000L
#endif	/*!FTHMSDOS*/
#endif	/*!FTHXEN286*/
#endif	/*!FTHSDM*/

	/* size of FAR memory area for MSDOS */
#ifdef	FTHMSDOS
#define FTGFARMEM	65500L
#endif

	/* largest catalog id (+1) */
#define FTCIDINF	((FTCID)0x7fffffff)

	/* PRIVATE special fieldid used by indexing programs - do not modify */
#define FTFIDXDEL	1
	/* PRIVATE special fieldid used to pass application data through the
	   virtual catalog functions without affecting indexing */
#define FTCVFID 	1
	/* minimum searchable fieldid, i.e:  */
	/* PRIVATE minimum valid internal id for indexing, searching */
#define FTFIDSMIN	2
	/* PRIVATE document length (== FTCICTIME) */
#define FTFIDDLEN	26
	/* minimum valid fieldid (except internal ids) */
#define FTFIDMIN	32
	/* maximum valid fieldid, maximum NXF-settable fieldid */
#define FTFIDINF	254
	/* maximum valid fieldid if FTCFGFID true in catalog bin zero */
#define FTFIDGINF	((FTFID)64010)
	/* first id for FX application-defined fields */
#define FTFIDFAFX	(FTFIDGINF+1)
	/* last id for FX application-defined fields */
#define FTFIDLAFX	(FTFIDGINF+1000)
	/* id for document weight field (ftfxcxtr()) */
#define FTFIDDWFX	(FTFIDLAFX+1)
	/* error return value from a routine returning FTFID data type */
#define FTFIDERR	((FTFID)65535)

	/* assorted catalog dimensions:
		FTCBNSZ - default size of catalog bins
		FTCMPSZ - default size of catalog map entries
		FTCMXBNS - default maximum number of bins per catalog entry
		FTCMXZSZ - default (and maximum) size of catalog record zero
		FTCMXFLDS - maximum number of application fields per catentry
		FTCMINFLDS - minimum number permitted for mxflds bin 0 entry
	*/
#define FTCBNSZ 	128
#define FTCMPSZ 	4
#ifdef	FTHMSDOS
#define FTCMXBNS	2
#else
#define FTCMXBNS	16
#endif
#define FTCMXZSZ	256
	/* maximum number of catalog fields */
#ifdef	FTHMSDOS
#define FTCMXFLDS	33
#else
#define FTCMXFLDS	253
#endif
#define FTCMINFLDS	4

/* Tell vector size for catalog read streams. */
#define FTCVTSZ		9

	/* maximum VCC (+1) */
#define FTVCCINF	((FTVCC)2147483647)
	/* maximum searchable VCC (+1) */
#define FTVCCSMAX	((FTVCC)16777216)

	/* maximum number of FTBBs per FTSB
	   (number of collections searchable simultaneously) */
#ifdef	FTHXLM
#define FTGBBSLIM	16
#else
#ifdef	FTHMSDOS
#define FTGBBSLIM	3
#endif
#endif
#ifndef	FTGBBSLIM
#define FTGBBSLIM	7
#endif

	/* maximum length of "mode" strings passed to ftbopen */
#define FTBBMODESZ	5

	/* maximum number of thesauri selected simultaneously */
#define FTGTHSLIM	1

	/* maximum number of lemma file selected simultaneously */
#define FTGLMSLIM	1

	/* maximum size of document filter list buffer */
#ifdef FTHMSDOS
#ifndef FTHWIN
#ifndef FTHOS2
#ifndef FTHNET
#define FTGMXFILT	32
#endif
#endif
#endif
#endif

#ifndef FTGMXFILT
#define FTGMXFILT	256
#endif

	/* maximum size of network filter list buffer */
#define FTNMXFILT	32

	/* maximum error message size */
#define FTGMSGSIZ	256
	/* maximum who string size (+1) */
#define FTGWHOSIZ	32

	/* PRIVATE maximum number of deleted documents/index pass */
#ifdef	FTHSDM
#ifdef	FTHWIN
#define FTDELMAX	2048
#else
#define FTDELMAX	4096
#endif	/*!FTHWIN*/
#endif	/* FTHSDM*/
#ifdef	FTHXLM
#define FTDELMAX	65536
#endif
#ifndef	FTDELMAX
#define FTDELMAX	((sizeof(int) == 2) ? 8191 : 16384)
#endif

	/* default filter list */
#define FTGDEFFILT	"s"

	/* document library header size-excluding any filename and final null */
#define FTLIBHDSZ	13

	/* maximum size of cmode,imode for ftbopen,ftbxopen */
#define FTGBCSIZ	5
#define FTGBISIZ	5

	/* maximum size of longs vector for ftbcreate() */
#define FTGCPARM	10
	/* maximum size of strings vector for ftbcreate() */
#define FTGSPARM	7
	/* maximum size of opt string for ftbcreate() */
#define FTGOPTSZ	4

	/* standard Ful/Text file names */
#ifndef FTHVMCMS
#define FTGFNPIM	"ftmess"
#define FTGFNWHO	"ftwhos"
#define FTGFNSTP	"fultext.stp"
#define FTGFNUIM	"fultext.msg"
#define FTGFNCTR	"fultext.ftc"
#else	/*FTHVMCMS*/
/* file names without extension or lowercase are not allowed */
#define FTGFNPIM	"FTMESS.VM"
#define FTGFNWHO	"FTWHOS.VM"
#define FTGFNSTP	"FULTEXT.STP"
#define FTGFNUIM	"FULTEXT.MSG"
#define FTGFNCTR	"FULTEXT.FTC"
#endif	/*FTHVMCMS*/

	/* standard Ful/Text extensions for various file types */
#ifdef FTHMVS
#define FTGEXTCFG	"CFG"
#define FTGEXTSTP	"STP"
#define FTGEXTMSG	"MSG"
#define FTGEXTTHS	"FTH"

#ifndef DATAMAT
#define FTGEXTLEM	"FTL"
#else /*DATAMAT*/
#define FTGEXTLEM	"FLH"
#define FTGEXTIDX	"FLI"
#define FTGEXTVOC	"VOC"
#define FTGEXTNEW	"NEW"
#define FTGEXTVNF	"VNF"
#define FTGNMREVV	"RGLV PCK"
#define FTGNMRENV	"RGLN PCK"
#endif /*DATAMAT*/

#else
#ifdef	FTHUNIX
#define FTGEXTCFG	".cfg"
#define FTGEXTSTP	".stp"
#define FTGEXTMSG	".msg"
#define FTGEXTTHS	".fth"

#ifndef DATAMAT
#define FTGEXTLEM	".ftl"
#else /*DATAMAT*/
#define FTGEXTLEM	".flh"
#define FTGEXTIDX	".fli"
#define FTGEXTVOC	".voc"
#define FTGEXTNEW	".new"
#define FTGEXTVNF	".vnf"
#define FTGNMREVV	"rglv.pck"
#define FTGNMRENV	"rgln.pck"
#endif /*DATAMAT*/

#else
	/* MSDOS, VMS, AOS/VS, VM */
#define FTGEXTCFG	".CFG"
#define FTGEXTSTP	".STP"
#define FTGEXTMSG	".MSG"
#define FTGEXTTHS	".FTH"

#ifndef DATAMAT
#define FTGEXTLEM	".FTL"
#else /*DATAMAT*/
#define FTGEXTLEM	".FLH"
#define FTGEXTIDX	".FLI"
#define FTGEXTVOC	".VOC"
#define FTGEXTNEW	".NEW"
#define FTGEXTVNF	".VNF"
#define FTGNMREVV	"RGLV.PCK"
#define FTGNMRENV	"RGLN.PCK"
#endif /*DATAMAT*/

#endif
#endif

#ifdef DATAMAT
	/* mode parameters for open() and creat():
		FTGOREAD	read
		FTGOCREA	create
	*/
#ifdef	FTHMSDOS
#define FTGOREAD	O_RAW
#define FTGOCREA	O_RAW
#else
#define FTGOREAD	0
#define FTGOCREA	0644
#endif
#endif /*DATAMAT*/

	/* mode parameters for fopen() or freopen():
		FTGFR:		read
		FTGFW:		write
		FTGFA:		append
		FTGFRU: 	read/update
		FTGFWU: 	write/update
		FTGFAU: 	append/update
	   FTGFMODL is the maximum length (+1) of the mode parameter
	*/
#ifdef	FTHMSDOS
#define FTGFMODL	4
#define FTGFR	"rb"
#define FTGFW	"wb"
#define FTGFA	"ab"
#define FTGFRU	"r+b"
#define FTGFWU	"w+b"
#define FTGFAU	"a+b"
#else	/* !FTHMSDOS */
#define FTGFMODL	3
#define FTGFR	"r"
#define FTGFW	"w"
#define FTGFA	"a"
#define FTGFRU	"r+"
#define FTGFWU	"w+"
#ifdef	FTHBSD
#ifdef  ultrix          /*90121001*/
#define FTGFAU	"A+"	/* at least Ultrix, if not others */
#endif
#endif
#ifndef	FTGFAU
#define FTGFAU	"a+"
#endif
#endif	/* !FTHMSDOS */

	/* flags for ftuclist() */
#define FTUCLLOC	0x0002

	/* node name separator character for collection name */
#define FTNSCNOD	'@'
	/* reserved node name means local node */
#define FTNLOCAL	'.'

	/* flags for ftufullp(), ftusfsch() */
#define FTGFUFO		0
#define FTGFUPF 	1
#define FTGFUFP 	2
#define FTGFURS		4		/* ftufullp() only */
#define FTGFUPO		8		/* ftusfsch() only */
#define FTGFUCO		16		/* ftusfsch() only */
#define FTGCRTP		32		/* ftufullp() only */

	/* constants for ftapiini() */
#define FTANLONG	9
/* longs[] index values */
#define FTALPLOS	0		/* minimum pool space */
#define FTALPHIS	1		/* maximum pool space */
#define FTALBHIS	2		/* maximum buffer space */
#define FTALBNUM	3		/* maximum buffer number */
#define FTALSHIS	4		/* maximum search space */
#define FTALCNUM	5		/* no. of catalog file sets */
#define FTALINUM	6		/* no. of index file sets */
#ifdef	FTHWIN
#define FTALWINS	7		/* Windows instance handle */
#else	/*!FTHWIN*/
#define	FTALSVLP	7		/* reserved for ftserver use */
#endif	/*!FTHWIN*/
#define FTALNOSV	8		/* services not initialized */
#define FTALNCON	9		/* maximum number of net connections */
	/* bit values for services not to be initialized */
#define FTASVSRCH	0x0001		/* search engine */
#define FTASVINDX	0x0002		/* indexing engine */
#define FTASVDLST	0x0004		/* standard directory list */
#define FTASVTEMP	0x0008		/* default directory for temp files */
#define FTASVMSG	0x0010		/* ftgmess() */

	/* flags for ftuspawn */
#define FTGBSHELL	1
#define FTGCSHELL	2
#define FTGNOFORK	4
#define FTGNOWAIT	8
#define FTGNOCLOSE	16
#define FTGASHELL	32
#define FTGNOVFORK	64
#define	FTGSWAPOUT	128		/* swap out calling program (DOSONLY) */

	/* flags for ftbistrt */
	/* migrate to ftitypes.h when ftbindex dropped */
#define FTBALL		0x00000001L
#define FTBNOCCHK	0x00000002L
#define FTBDEBUG	0x00000004L
#define FTBVERBOSE	0x00000008L
#define FTBREWLOG	0x00000010L
#define FTBIMSG 	0x00000020L
#define FTBSTPOK	0x00000040L
#define FTBDYXINIT	0x00000080L
#define FTBREDO 	0x00000100L
#define FTBPRIV 	0x00000200L
#define FTBSTPCK	0x00000400L
#define FTBCANCEL	0x00000800L	/* PRIVATE */
#define FTBIQUIT	0x00001000L
#define FTBUNLOCK	0x00002000L
#define FTBNOIWRD	0x00004000L
#define FTBNOIBLD	0x00008000L
#define FTBNEWNDX	0x00010000L
#define FTBSHARE	0x00020000L

	/* flags for sflags element of FTSDATA structure */
#define FTSQUICK	0x0001

	/* flags for ftshstrt(), ftsopen(), ftsclose() */
#define FTSQNULL	0
#define FTSQFILE	1
#define FTSQFTFB	2
#define FTSDEBUG	4
#define FTSQSTRN	8
#define FTSRESFN	0x10
#define FTSQUNLINK	0x20
#define FTSMEMTELL	0x40
#ifdef FTSVSS
#define FTSQCID		0x80	/* ONLY FOR ftqhstrt() */
#define	FTSQNAME	0x100
#define	FTSOOBJ		0x200
#endif /* FTSVSS */
#define	FTSNOSR		0x400
/* PRIVATE */
#define FTSBEKEEP	0x80	/* used only by ftsopen(), for
				   backwards compatibility */

	/* flags for ftsstat(),ftsrstat() */
#define FTSNCOLL	0x01
#define FTSRBKTG	0x02
#define FTSRLSZ 	0x04
#define FTSRC		0x08
#define FTSNDOCS	0x10
#define FTSCCID 	0x20
#define FTSCNDX 	0x40
#define FTSVDOCS	0x80

	/* size of buffer required for ftsrstat() */
#define FTSRBKSZ	FTNPATHSZ

	/* maximum size of buffer required for ftsrlink() */
	/* NOTE: preferred method is to ask ftsstat() for buffer size */
#define FTSRBKMX	(6+FTNPATHSZ+(6*FTGBBSLIM))

	/* catalog field ids */
#define FTCDDMIN	27
#define FTCDDATE	31
	/* fields "magically" defined by ftccheck during directory walk */
#define FTCUOWNER	97
#define FTCUDNAME	100
#define FTCUFNAME	110
	/* PRIVATE catalog field ids */
#define FTCFNAME	3
#define FTCDTYPE	4
#define FTCMTIME	5
#define FTCSTORE	6
#define FTCTXTID	7
#define FTCSDFLG	8
	/* PRIVATE DTD entry */
#define FTDTDFID	22
	/* PRIVATE binary version of FID 31 (internal to s/w only) */
#define FTCBDATE	23
	/* PRIVATE document segment markers */
#define FTCDMARK	24
	/* PRIVATE fticheck() FTCDIR UNIX optimization */
#define FTCINODE	25
	/* PRIVATE cat entry time stamp (== FTFIDDLEN) */
#define FTCICTIME	26
	/* Note that the following "fields" aren't catalog fields at all;
	   they are used by the standard user interface to format various
	   templates for display.
		FTVCNAME	PRIVATE collection name
		FTVNDOCS	PRIVATE total number of documents retrieved
		FTVSEQN 	PRIVATE topic list sequence number
		FTVDFLAG	PRIVATE document status flag
		FTVVDOCS	PRIVATE vector of documents retrieved
		FTVPROXD	PRIVATE proximity distance
		FTVSCANI	PRIVATE search cancel interval
		FTVTHESO	PRIVATE thesaurus
		FTVSDATE	PRIVATE system date
		FTVSTIME	PRIVATE system time
		FTVLFILE	PRIVATE lemma file
		FTVLEVEL	PRIVATE lemma expansion level
		FTVTLEFT	PRIVATE text to left of display
		FTVTRGHT	PRIVATE text to right of display
		FTVLNMBR	PRIVATE line number
		FTVCNMBR	PRIVATE column number
		FTVDWGHT	PRIVATE document weight
		FTVRNKWT	PRIVATE ranking specification for doc weight
		FTVRNKDT	PRIVATE ranking specification for date order

		FTVBMTOP	PRIVATE topic profile associated with bookmark
		FTVBMDOC	PRIVATE document select index or catalog id
				associated with bookmark
		FTVBMFRM	PRIVATE frame number associated with bookmark
		FTVCSIZE	PRIVATE collection size
		FTVFNAME	PRIVATE system file name
		FTVFLIST	PRIVATE filter list
		FTVSFILE	PRIVATE stopfile
		FTVMFILE	PRIVATE message file
		FTVPATH 	PRIVATE PTH config file entry
		FTVCDIR 	PRIVATE directory collection resides in
		FTVITYPE	PRIVATE collection's index type
	*/
#define FTVCNAME	9
#define FTVNDOCS	10
#define FTVSEQN 	11
#define FTVDFLAG	12
#define FTVVDOCS	13
#define FTVPROXD	14
#define FTVSCANI	15
#define FTVTHESO	16
#define FTVSDATE	17
#define FTVSTIME	18
#define FTVLFILE	19
#define FTVLEVEL	20
#define FTVTLEFT	21
#define FTVTRGHT	22
#define FTVLNMBR	23
#define FTVCNMBR	24
#define FTVDWGHT	25
#define FTVRNKWT	26
#define FTVRNKDT	27
/* the numbers 10-25 are re-used here */
#define FTVCINT 	10
#define FTVCSTAT	11
#define FTVCTERM	12
#define FTVCZONE	13
#define FTVCBINS	14
#define FTVCEBIN	15
#define FTVHICID	16
#define FTVCTYPE	17
#define FTVBSRCH	18
#define FTVBMTOP	19
#define FTVBMDOC	20
#define FTVBMFRM	21
#define FTVCSIZE	22
#define FTVFNAME	23
#define FTVFLIST	24
#define FTVSFILE	25
#define FTVMFILE	26
#define FTVPATH 	27
#define FTVCDIR 	28
#define FTVITYPE	29
/* don't exceed 31 */

	/* PRIVATE catalog formatting codes for ftcffmt() */
#define FTCNULLT	1
#define FTCFILLB	2
#define FTCRJUST	4
#define FTCFILLZ	8

	/* date conversion and formatting codes
		FTUSDATE, FT4YRDATE, FTSLSHDATE, and FTISODATE can be used
		with the date formatting function ftudntos() or with ftcffmt()
	*/
#define FTUSDATE	16
#define FT4YRDATE	32
#define FTSLSHDATE	64
#define FTISODATE	128
#define FTVFYDATE	256
#ifndef FTUDATE
#define FTUDATE 	1
#define FTFDATE 	2
#define FTNDATE 	4
#define FTFMDATE	8
#define FTFUDATE	512
#define FTCTIME		1024	/* gets ftudntos() to emulate UNIX ctime() */
#define FTCMXDATE 20471231
#define FTCMXIDAT 1048479
#endif

        /* dictionary entry keyword for numeric reference string */
        /* - for FTCDDATE field/zone only */
#define FTGNDATW        "\116"  /* "N" */
#define FTGNDATC        '\116'  /* 'N' */
        /* for all zones except FTCDDATE */
#define FTGNINDW        "\010"
#define FTGNINDC        '\010'

	/* ftnxtc() flags */
/* NOTE: FTFFTCS can also be used with ftucseq() */
#define FTFFTCS		0x02	/* return char in FTCS */
#define FTFNCS		0x04	/* return char in NCS */

	/* catalog entry status flags */
#define FTCMRK		1
#define FTCNDX		2
#define FTCNIN		4
#define FTCLIB		8
#define FTCBUSY 	16
#define FTCDIR		32
#define FTCDYNDX	64
	/* the following are used by ftcread(), but never written to MAP */
#define FTCFCFAIL	0x80		/* ftbfatr() failed */
#define FTCFCREAD	0x100		/* FTUFREAD not set */
			/* 0x200 unused */
#define FTCFCDATE	0x400		/* modified date different */
#define FTCFCMASK	0x580		/* mask to get extra bits */
	/* the following are set by ftshcfld, but not written to MAP */
	/* in each case, (cistatus & 0x0fff) == fterrno */
	/* if FTCEFATAL is set, fterrno is fterrsub if FTCENETIO also set */
#define FTCRDFAIL	0x8000		/* recoverable failure in ftcsread() */
#define	FTCEFATAL	0x4000		/* fatal error */
#define	FTCENETIO	0x2000		/* set with FTCEFATAL if FTENETIO */
	/* the following is used by ftcwrt(), but not written to MAP */
#define FTCNUPDYX	0x80		/* don't update immediate index */

	/* catalog sub-document descriptor flags */
#define FTCSDDFDT	1		/* default f 31 to csmtime */
#define FTCSDWLK	2		/* subtree has been walked */
#define FTCDDTD		4		/* entry is a DTD */
#define FTCSDNOSUB	16		/* do not traverse subdirectories */
	/* the following added to get ftcwrt() to perform
	   file checking and relativization; they are not written to the
	   catalog nor returned by ftcread() */
#define FTCSDFCHK	4		/* perform file status check */
#define FTCSDFREL	8		/* relativize fn by current dir */
#define FTCSDPREL	32		/* relativize fn by PTH */
/*	FTSHCXLAT	0x8000		   PRIVATE: set by ftshcfld() */

	/* catalog duplicate checking modes for ftcdup() */
#define FTCDUPDOC	0
#define FTCDUPDIR	1
#define FTCDUPSD	2
#define FTCDUPLIB	3
#define FTCDUPLDOC	4

	/* flags for ftbstat() */
#define ftcstat(bb,stid,area)	ftbstat(bb,stid,(area))
	/* the first bunch also passes through to ftcvstat() */
#define FTCHICID	1
#define FTCMAPSZ	2
#define FTCVCCTAB	3
#define FTCLOCK 	4
#define FTCEXTF 	5
#define FTCINDA 	6
#define FTCNOSTAT	7
#define FTCRECSZ	8
#define FTCFILESZ	9	/* PRIVATE total CAT, CIX file size */
#define FTCULMT		10
#define FTCZEROSZ	16
#define FTCFLAGS	32
#define FTCCHKDAT	64
#define FTCVER		128
	/* this bunch is only used by directly calling ftcvstat() */
#define FTCHIBIN	129 	/* PRIVATE highest bin number in use */
#define FTCEXCL		130	/* Turbo catalog mode request */
#define FTCGFID		131	/* Get value of FTCFGFID flag */

	/* this next bunch is handled without calling ftcvstat() */
#define FTBSEARCH	200
#define FTBYNDSZ	201
#define FTBIMMED	202
#define FTBWRT		203
#define FTBNODENM	204
#define FTBNXL		205
#define FTBAOPT		206
#define FTBPTH		207
#define FTBNCOMP	209
#define FTBREMOTE	210
#define FTCFFBFSZ	211
#define FTCDMBFSZ	212
#define FTCNFFLDS	213
#define FTCFXRNG	(1<<15)
#define FTCVRRNG	(1<<14)
#define FTCVCCINC	(1<<13)
#define FTCCATFMT	(1<<12)
#define FTCMAXBIN	(1<<11)
#define FTCBINSZ	(1<<10)
#define FTCMAXFLDS	(1<<9)
#define FTCINTEG	(1<<8)

	/* bits within FTCFLAGS flags */
	/* note that these are also used within the catalog functions */
	/* to map catalog bin zero flags (FTCVIOB.cvflags) */
	/* note interrelationship between these bits and dictionary header */
	/* bits mapped in "dcttypes.h" */
	/* note FTPLOCKED lock flag has a value of 1 */
	/* hi-integrity mode */
#define FTCHIINT	2
	/* substitution of library file offset for CID */
#define FTCCIDOFF	4
	/* dictionary contains term occurrence statistics */
#define FTCIVAL		8
	/* allow increase of maxbins and maxflds values */
#define FTCFDSTAT	0x10
	/* catalog contains at least one entry with cinsdocs > 0 */
#define FTCFEXTF	0x20
	/* terms are sorted word-order in the dictionary */
#define FTCFTWOR	0x40
	/* giant (2-byte) field IDs and zones */
#define FTCFGFID	0x80
	/* The following flags are not actually written to bin zero */
	/* catalog file resides on an NFS filesystem */
#define FTCNFS		0x100
	/* the collection is read-only (no file stat'ing required) */
#define FTCWORM		0x200
	/* catalog exclusively locked (turbo catalog mode) */
#define FTCFEXCL	0x400
	/* bin 0 was read and is locked. No need to read it again. */
#define FTCB0RED	0x800
	/* catalog is in static format */
#define FTCSTATIC	0x1000
	/* Bits which are ok to send to ftcvinit - ie. useful for re-creation */
#define FTCINIOK	(FTCHIINT|FTCCIDOFF|FTCFDSTAT|FTCFTWOR|FTCFGFID)

	/* mode values ftblock() */
#define FTBLKON 	1		/* set collection lock */
#define FTBLKOFF	2		/* clear collection lock */
#define FTBLKSTAT	3		/* get current lock status */
#define FTBLKHON	4		/* set hi-integrity mode */
#define FTBLKHOFF	5		/* clear hi-integrity mode */
#define FTBLKHSTAT	6		/* get current H.I. mode */

	/* flags for ftcrewrt() */
#define FTCMERGE	1
#define FTCRERD 	2
#define FTCWSUBD	4

	/* Mode flags for catalog stream reading */
#define FTCNONE		0x00	/* Return sub-document info only */
				/* NOTE: FTCNONE is not legal at the virtual
				   layer */
#define FTCINCL		0x01	/* Return only fields in list */
#define FTCXCL		0x02	/* Return only fields not in list */
#define FTCALL		0x03	/* Return all fields */
#define FTCOONE		0x08	/* Only return one of each FID */
	/* More mode flags - internal use only */
#define FTCVSD		0x04	/* Always return sd info */
#define FTCVSFRE	0x01	/* Start allocation on free list */
#define FTCVNOUL	0x02	/* Don't unlock on write */
#define FTCVREL		0x10	/* Release bins on ftfclose */
#define FTCDTD		0x20	/* Read a DTD entry */

	/* flags for ftudatr() */
#define FTUDFREE	1
#define FTUDBPC 	2
#define FTUDINOD	4
#define FTUDNAME	8
#define FTUDISRO	16

	/* flags for ftufatr() */
	/* first three must correspond to access() mode argument */
#define FTUFEXEC	1
#define FTUFWRITE	2
#define FTUFREAD	4
#define FTUFRID 	8
#define FTUFDIR 	16
#define FTUFREG 	32
#define FTUFDATE	64
#define FTUFSIZE	128
#define FTUFUID 	256
#define FTUFGID 	512
#define FTUFDWRITE	1024
#define FTUFPNAM	2048
#define FTUFISRFD	4096
#define FTUFCTIME	8192
#define FTUFDEV        16384

	/* flags for ftfropen() */
	/* ftfropen() also accepts FTUFWRITE, FTUFREAD and FTUFEXEC */
	/* caller doesn't want central I/O manager services */
#define FTUFLOCAL	8
	/* debugging trace desired for each ftfrio() */
#define FTUFDEBUG	16
	/* create only if file does not exist */
#define FTUFCREAT	32
	/* caller will use ftfrlock() with this file */
#define FTUFLOCK	1024
	/* open will fail if file exists */
#define FTUFNEW 	2048
#define FTUFRAW 	4096
	/* position to current EOF */
#define FTUFAPND	8192
	/* create file if non-existant, otherwise truncate existing file */
#define FTUFCROP	16384
	/* make file only accessable to current user */
#define FTUFPROP	32768
	/* vms only: not RMS STREAM-LF file */
#define FTUFRMS 	65536

	/* mode parameter for ftfrio(), ftfrlock() */
	/* note FTUFREAD, FTUFWRITE also used */
#define FTLOCKB		8			/* blocking lock */
#define FTLOCKNB	16			/* non-blocking lock */
#define FTLOCKR		32			/* read lock */
#define FTLOCKRB	(FTLOCKB|FTLOCKR)	/* blocking read lock */
#define FTLOCKRNB	(FTLOCKNB|FTLOCKR)	/* non-blocking read lock */
#define FTLOCKA		64			/* entire file */
#define FTLOCKU		1			/* unlock */
#define FTLOCKUA	(FTLOCKU|FTLOCKA)	/* unlock entire file */
#define FTLOCKBA	(FTLOCKB|FTLOCKA)	/* blocking lock entire file */

#ifndef	ix370
	/* old versions (not IX/370 compatible) */
#define FTLOCK_B	8			/* blocking lock */
#define FTLOCK_NB	16			/* non-blocking lock */
#define FTLOCK_R	32			/* read lock */
#define FTLOCK_RB	(FTLOCK_B|FTLOCK_R)	/* blocking read lock */
#define FTLOCK_RNB	(FTLOCK_NB|FTLOCK_R)	/* non-blocking read lock */
#define FTLOCK_A	64			/* entire file */
#define FTLOCK_U	1			/* unlock */
#define FTLOCK_UA	(FTLOCK_U|FTLOCK_A)	/* unlock entire file */
#define FTLOCK_BA	(FTLOCK_B|FTLOCK_A)	/* blocking lock entire file */
#endif

	/* used by ftcread() ftcsget() */
#define FTCLOCKM 	(FTLOCKB|FTLOCKNB|FTLOCKR)/* Mask for all possible */
						/* catalog lock flags */
#define FTCSEEK 	FTLOCKU			/* Get CID supplied */
#define FTCPREV 	0x80			/* Get prev CID */
#define FTCNEXT 	0x200			/* Get next CID */
#define FTCFIRST 	0x400			/* Get first CID */
#define FTCEND 		0x800			/* Get last CID */
#define FTCPOSM	(FTCSEEK|FTCPREV|FTCNEXT|FTCFIRST|FTCEND)
#define FTCNRD		0x1000			/* don't read catalog data into
						   common buffer */

	/* number of catalog bins read into buffer at a time */
#define FTNBINS		8

	/* used by ftcread() */
#define FTCFCHK 	FTLOCKA			/* update status by ftufatr() */

	/* flag for ftugfoog() */
#define FTUFOWNER	1

	/* flags for flutils (ftufcopy(), ftufmove(), ftumkdir()) */
#define FTUCCDIR	1
#define FTUCMTIM	2
#define FTUCNCLB	4
#define FTUCPRSV	8

	/* PRIVATE flags for ftflchk */
#define FTGCHKNAM	1
#define FTGCHKMAG	2

	/* PRIVATE flags for ftugstr() */
#define FTGESCAPE	1
#define FTGSKPLTC	2		/* skip leading terminators */

	/* flags for ftpminit() */
#define FTPMFARM	1

	/* flags for ftfuopen() */
#define FTFUNCNXT	0x100		/* don't close next filter */
#define FTFUCATLG	0x200		/* stream comprises catalog data */
#define FTFUTRUBL	0x400		/* truncate trailing blanks */
#define FTFUBEDRF	0x800		/* stream contains embedded references
					   of the form %reference;label% */
#define FTFUNLSP	0x2000		/* treat newline like space */
#define	FTFUBM		0x4000		/* private			*/

	/* symbols for filter extension functions */
	/* flags for ftfxcget(), ftfxdopn(), ftfxpmrg() */
#define FTFXCERD	1		/* catalog entry already read */
#define FTFXSROK	2		/* search result check ok */
#define FTFXMDOK	4		/* last-modified-date check ok */
#define FTFXNDOK	8		/* not-deleted check ok */
#define FTFXIXOK	0x10		/* is-indexed check ok */
#define FTFXNBOK	0x20		/* not-busy check ok */
#define FTFXXVCC	0x40		/* ignore VCC information in s.r. */
#define FTFXITOK	0x80		/* is-text check ok */
#define FTFXBM		0x4000		/* private			*/
	/* flag for ftfxcxtr() */
#define FTFXRVRS	1		/* go in reverse */
	/* whence for ftfxvpos() */
#define FTFXWBEG	0x80		/* set vcp to beginning + offset */
#define FTFXWCUR	0x100		/* set vcp to current + offset */
#define FTFXWEND	0x110		/* set vcp to end + offset */
	/* unit for ftfxvpos() */
#define FTFXULIN	1		/* unit is a line */
#define FTFXUHIT	2		/* unit is hilite-line */
#define FTFXUWRD	3		/* unit is a word */
#define FTFXUCHR	4		/* unit is a character */
	/* infovec[] subscripts and dimension for ftfxlget() */
#define FTFXIFLG	0		/* infovec[0]: flags */
#define FTFXINVC	1		/* infovec[1]: # of visible chars. */
#define FTFXINWD	2		/* infovec[2]: # of words */
#define FTFXIBCT	3		/* infovec[3]: buffer byte count */
#define FTFXINGR	4		/* infovec[4]: # of GR changes */
#define FTFXINCS	5		/* infovec[5]: # of other c-seqs */
#define FTFXINEL	6		/* max. number of infovec elements */
	/* infovec[FTFXIFLG] for ftfxlget() */
#define FTFXLAST	1		/* last line in stream */
#define FTFXFORM	2		/* line followed by formfeed */
#define FTFXHLIN	4		/* line is a hilite-line */
#define FTFXSUPC	8		/* line contains supplemental chars */
#define FTFXNEWL	16		/* line followed by newline */
#define FTFXOFLO	32		/* user's buffer was too small */
#define FTFXBELE	64		/* begin element sequence in stream */	
#define FTFXEELE	128		/* end element sequence in stream */
	/* symbols for field exit functions */
#define FTFXFINIT	0		/* initialize */
#define FTFXFFBEG	1		/* begin field */
#define FTFXFTELL	2		/* record position */
#define FTFXFSEEK	3		/* return to recorded position */
#define FTFXFFGET	4		/* get field data */
#define FTFXFTERM	5		/* terminate */
#define FTFXFSLCT	6		/* select new instance */
	/* flag for ftfxtopn() */
#define FTFXTXFIX	1		/* drop fixed text from stream */
	/* query translation codes for ftfuqfmt(), ftfuqget() */
#define FTFXQNOTR	0		/* no translation */
#define FTFXQMETA	1		/* meta-character substitution */
#define FTFXQLITL	2		/* literal field */
#define FTFXQDATE	3		/* date field */
	/* template identifiers */
#define FTFXQFTID	0x4000		/* first query template id */
	/* flag for ftfxhini() */
#define FTFXHFTFB	1		/* 'path' parameter is FTFB pointer */
	/* flags for ftfxhopn() */
#define FTFXHGLOS	1		/* item is a glossary */
#define FTFXHTPIC	2		/* item is a topic */
#define FTFXHALIS	4		/* item is an alias */
#define FTFXHINDX	8		/* use index value for selection */
	/* flags for ftshcfld() */
#define	FTSHCFSEL	1		/* selected fields */
#define	FTSHCFALL	2		/* all fields */
#define	FTSHCXLAT	0x8000		/* PRIVATE: set this same flag in
					   sub-doc csflags if ftbopen(,"n",)
					*/

	/* query analysis parameters */

/* value for normalizing a document weight -- statistical ranking */
#define	FTGSMXWT	1000
/* maximum user term weight */
#define FTQMAXSTW	1000

/* maximum distance between operands in phrase or proximity */
#define FTQDISTM	65530		/* must fit in USHORT */

#define FTQMXSBSZ       4   /* arbitrary maximum number of characters in a */
                        /* lemma substitution, including marker characters */

#define FTQLMXENTRY     40     /* arbitrary limit on the number of entries */
        		   /* in a substitution table,actually FTQLMXENTRY */

#define FTQVRPRWRD      30    /* arbitrary maximum number of substitutions  */
			     /* allowed in a word                          */

	/* maximum lemma explosion level */
#define FTQEXPLM	2

	/* maximum number of zone ranges selectable by one zone operator */
#define FTQMAXFR	20

	/* catalog stream access modes for ftcopen() and ftcreopn() */
#define FTCWRT		2
#define FTCRDONLY	4
#define FTCREWRT	8
#define FTCRAW		32  /* Don't convert file names - currently unused */
#define FTCSTRM		128  /* Use stream mode */
#define FTCBUF2		256  /* Allocation 2nd buffer */
#define FTCOVWRT	512  /* Use faster non-secure writes */
	/* catalog stream access mode for ftcreopn() only */
#define FTCATCLSE	16
#define FTCDYNBUF	64

	/* parameters for control sequences in standard document format */
	/* maximum parameter value in a control sequence		*/
#define FTICSPMAX	(unsigned)0xfff0
	/* special characters which may occur in a control sequence	*/
#define FTICSPCOL	(unsigned)0xffff	/* : */
#define FTICSPSEM	(unsigned)0xfffe	/* ; */
#define FTICSPLT	(unsigned)0xfffd	/* < */
#define FTICSPEQL	(unsigned)0xfffc	/* = */
#define FTICSPGT	(unsigned)0xfffb	/* > */
#define FTICSPQMK	(unsigned)0xfffa	/* ? */
	/* default VCC increment					*/
#define FTIVIDEF	1000
	/* maximum VCC increment in any one control sequence		*/
#define FTIVIMAX	65000

	/* PRIVATE flags for ftuwild() package */
#define FTUWRELNM	1
#define FTUWXLAST	2
#define FTUWRECUR	4
#define FTUWALPHA	8
#define FTUWDSCND	0x10
#define FTUWDEPTH	0x20
#define FTUWKEEPS	0x40

	/* PRIVATE filenames for virtual files */
#define FTFVQUERY	"Q"
#define FTFVDOSP	"\001"
#define FTFVSAVE	"?"
#define FTFVDBLS	"L"
#define FTFVFOO 	"x"

	/* flags for ftucseq() */
	/* NOTE: FTFFTCS is also valid! */
#define FTUCCOPY	4
#define FTUCRCOV	8	/* private				*/

	/* maximum number of elements in CSI vector, for ftucseq() */
#define FTUCNELM	80

	/* size of signal handler array for ftustrap(), ftusrest() */
#ifdef	FTHNOSIG
	/* array may get declared even where not needed */
#define	FTUSIGNM 1
#else	/*!FTHNOSIG*/
#ifdef	FTHMSDOS
#define	FTUSIGNM 2
#else
#define	FTUSIGNM 4
#endif
#endif	/*!FTHNOSIG*/

	/* document state attributes (must be in 0xff00 range) */
#define FTAXNDX 0x100	/* suppress indexing */
#define FTAXVCC 0x200	/* suppress VCC incrementing */
#define FTAXDSP 0x400	/* suppress display */
#define FTAXLIT 0x800	/* enable literal mode (disabled by default) */

	/* sizes for compiled tables in DCT header */
#define FTCSETSZ	256
#define FTNXFSZ 	FTFIDINF+2

	/* flags for ftxopen() */
#define FTXUNIQ 	1	/* exact word browse  */
#define FTXSTEM 	2	/* word prefix browse */
#define FTXFLLW 	4	/* browse for all words following prefix    */
#define FTXBOTH 	8	/* browse both periodic and dynamic indexes */
#define FTXPRDC 	0x10	/* browse only the periodic index   */
#define FTXDYMC 	0x20	/* browse only the dynamic  index   */
#define FTXWRDC 	0x40	/* ftxgetw returns word occurrences */
#define FTXMSCND	0x80	/* specify canint value in milliseconds */

#define FTXSTPWRD	-1	/* value returned by ftxgetw for stop words */

	/* selectors for ftxstat() */
#define FTXDCTFMT	1	/* DCT format code */
#define FTXDCTFLG	2	/* DCT statistics */

	/* define native translation identifier */
#ifdef	FTHMSDOS 	/* MS-DOS */
#define FTHNCSID 	'd'
#else	/*!FTHMSDOS*/
#ifdef	vms		/* VT 220 */
#define FTHNCSID 	'v'
#else	/*!vms*/
#ifdef	mac		/* Macintosh */		/*90111601*/
#define FTHNCSID 	'c'
#else	/*!mac*/
#ifdef	FTHVMCMS	/* EBCDIC */
#define FTHNCSID	'm'
#else	/*!FTHVMCMS*/
#ifdef	FTHMVS		/* EBCDIC */
#define FTHNCSID 	'm'
#else	/*!FTHMVS*/	/* vanilla ASCII */
#define FTHNCSID 	'a'
#endif	/*!FTHMVS*/
#endif	/*!FTHVMCMS*/
#endif	/*!mac*/
#endif	/*!vms*/
#endif	/*!FTHMSDOS*/

	/* define string translation macros for NCS <-> FTCS */
#define FTFTCS	0x01	/* indicates that source char set is FTCS */
#define FTFBFS  0x02    /* indicates that '\F' support is enabled */
#define ftntft(a,b,c,d) (ftntstr((a),(b),(c),(d),0,FTHNCSID))
#define ftftnt(a,b,c,d) (ftntstr((a),(b),(c),(d),FTFTCS,FTHNCSID))

extern	void	ftucore();

/* defines for data types                                           */
#define FTGD_CHAR       1
#define FTGD_NUMERIC    2
#define FTGD_DECIMAL    3
#define FTGD_INTEGER    4
#define FTGD_SMALLINT   5
#define FTGD_FLOAT      6
#define FTGD_REAL       7
#define FTGD_DOUBLE     8
#define FTGD_VARCHAR    12
/* The following data types parallel Everest extensions to the SQL Access CLI */
#define FTGD_DATE       (FTICSPMAX-1)
#define FTGD_TIME       (FTICSPMAX-2)
#define FTGD_DATETIME   (FTICSPMAX-3)
#define FTGD_FTCHAR     (FTICSPMAX-4)
#define FTGD_FTVARCHAR  (FTICSPMAX-5)
#define FTGD_APVARCHAR  (FTICSPMAX-6)
#define FTGD_BINARY     (FTICSPMAX-7)

#endif
