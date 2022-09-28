/*static char *sccsid="@(#) ftnconst.h (1.3)  14:21:07 07/08/97";

   ftnconst.h -- constants for network layer

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------
  | MM  5.1B 91/10/10 | fix up protocol versions
  | MM  5.1B 91/08/30 | Add FTNUFFN 
  | MM  5.1B 91/08/19 | Add FTNDTRD, FTNDTWR
  | hjb 5.1B 91/08/19 | 91071806,91072501: FTNV51B4
  | wtr 5.1A 91/05/14 | Get rid of nested comments
  | cps 5.1A 91/02/20 | add FTNQHSTRT, FTNQACAN, FTNQINFO
  | hjb	5.0W 91/02/05 | add FTNFBMOV(moveable buffer), FTNOEPKT(no n_epkt)
  | bf  5.0D 90/09/02 | increment FTNREV for FTNSMCLSE flags
  | bf  5.0D 90/08/27 | add FTNFLAGS,FTNSMOPEN,FTNSMCLSE,FTNSMKEEP;drop FTNSSTAT
  | bf  5.0D 90/08/17 | FTNSFTSES
  | swr 5.0D 90/08/10 | Mac: make FTNSCPTH same as FTHPNSC for consistency
  | bf  5.0D 90/08/08 | define FTNFCLOS,FTNAUTOC; drop FTNMXCONN
  | cs  5.0D 90/08/01 | FTNREV to 2
  | bf  5.0D 90/07/31 | add FTNREV; drop old FTNV... constants; upgrade FTNVER
  | KRL 5.0C 90/06/08 | FTNMXCONN increased to FTGBBSLIM
  | DG	5.0C 90/05/29 | increment FTNV50C2 for ftsropen() change
  | bf  5.0C 90/05/01 | increment FTNV50C2 for another ftufgfnd change
  | bf  5.0C 90/04/23 | upgrade FTNVER to FTNV50C2 for ftufgfnd,ftbcreate mods
  | bf  5.0C 90/04/23 | move FTNSCNOD to ftgconst.h
  | bf  5.0C 90/04/11 | increment FTNV50C1 for ftuclist change
  | cs  5.0C 90/03/29 | Change FTNSGETC to FTNSRNNCD, remove FTNSSEEK, FTNSTELL
  | bf  5.0C 90/03/21 | FTNSGIDX
  | bf  5.0C 90/03/19 | FTNSC* separator characters
  | PW  5.0C 90/03/16 | FTNXSTAT
  | cs  5.0C 90/03/16 | changes for support of search id
  | bf  5.0C 90/03/14 | FTNFNODE,FTNFCOLL
  | DG	5.0C 90/03/06 | FTNSHCFLD replaces FTNXCXTR
  | PW  5.0C 90/03/08 | FTNXCLOSE
  | PW  5.0C 90/03/07 | FTNXSEEK
  | PW  5.0C 90/03/06 | FTNXTELL
  | PW  5.0C 90/03/05 | FTNXGEZ
  | PW  5.0C 90/03/02 | FTNXGETW
  | PW  5.0C 90/02/28 | FTNXOPEN
  | bf  5.0B 90/02/01 | FTNSSTAT
  | bf  5.0B 90/02/01 | upgrade FTNVER for client nodename in connect request
  | DG	5.0B 90/01/25 | FTNSTELL
  | bf  5.0B 90/01/05 | FTNSAKIL; upgrade FTNVER for multi-node search
  | AH  5.0B 89/12/28 | Added catalog stream mode functions
  | bf  5.0A 89/12/12 | upgrade FTNVER for async search
  | bf  5.0A 89/12/07 | rename FTNSOPEN to FTNSHSTRT
  | bf  5.0  89/11/13 | upgrade FTNVER for async protocols
  | bf  5.0  89/11/02 | new defines for asynchronous indexing & searching
  | bf  4.6C 89/10/16 | upgrade FTNVER for NULL cfld protocol change (#89101301)
  | bf  4.6C 89/08/03 | FTNBFREL
  | bf	4.6  89/03/02 | FTNFLSEP moved from ftntypes.h
  | bf  4.6  89/01/12 | FTNSCOPEN
  | bf  4.6  88/11/24 | upgrade FTNVER for ftnprntf "e" format
  |-------------------|---------------------------------

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */


#ifndef ftnconst_h
#define ftnconst_h

	/* current version and revision of network protocol */
	/* ftnconn.c and ftserver.c must be recompiled if either is changed */
#define	FTNVER		FTNV51B4
#define FTNREV		4
	/* codes for all protocol versions */
#define FTNV50C2	13
#define FTNV50D1	14
#define FTNV50F1	15
#define	FTNV51B4	51

	/* separator characters for FTNPATH */
	/* nested network filter character */
#define FTNSCFIL	':'
	/* FTNPATH component character */
#ifdef	mac
#define FTNSCPTH	FTHPNSC
#else
#define FTNSCPTH	';'
#endif
	/* nodename/filterlist */
#define FTNSCNFL	'/'

	/* flags for ftserver, passed with FTNCONN request */
#define FTNSFTSES	1

	/* flags for ftngnfb() */
#define	FTNOPEN		1
#define FTNSERVE	2

	/* flags for directory services "list" function */
#define FTNFNODE	1
#define FTNFCOLL	2

	/* flags for packet header */
#define	FTNASYNC	1
#define	FTNSYNC		2

	/* flags for nfb->n_flags */
#define FTNFCLOS	1
#define	FTNFBMOV	2

	/* stid values for ftnstat,ftnset */
#define FTNFLAGS	1

	/* auto-close prefix for network filter list */
#define FTNAUTOC	'!'

	/* sentinel value to indicate no n_epkt [formerly (char *)NULL] */
#define	FTNOEPKT	-1

	/* REQUEST TYPES */
	/* programmer interface function requests */

	/* collection requests */
#define FTNUFGFND	0	/* ftnfgfnd() */
#define FTNBOPEN	1	/* ftbopen() */
#define FTNBCLOSE	2	/* ftbclose() */
#define FTNBSTAT	3	/* ftbstat() */
#define FTNBSET		4	/* ftbset() */

	/* catalog requests */
#define FTNCOPEN	5	/* ftcopen() */
#define FTNCCLOSE	6	/* ftcclose() */
#define FTNCSGET	7	/* ftcsget() */
#define FTNCREAD	8	/* ftcread() & ftcsread() */
#define FTNCSEEK	9	/* ftcseek() */

#define FTNCREOPN	10	/* ftcreopn() */
#define FTNCSDUP	11	/* ftcsdup() & ftcssdup() */
#define FTNCWRT		12	/* ftcwrt()  & ftcswrt() */
#define FTNCSPUT	13	/* ftcsput() */
#define FTNVMGET	14	/* MSG file kluge for old UI */
#define FTNCRLSE	15	/* ftcrlse() */
#define FTNDTRD		61	/* ftdtrd() */
#define FTNDTWR		62	/* ftdtwr() */

	/* document requests */
#define FTNBFATR	16	/* ftbfatr() */
#define FTNBFOPEN	17	/* ftbfopen() */
#define FTNBFCLSE	18	/* ftfclose() */
#define FTNBFSEEK	19	/* ftfseek() */
#define FTNBFFBUF	20	/* ftfb_fbuf() */
#define	FTNBFPBUF	21	/* ftfb_pbuf() */
#define FTNBFREL	40	/* ftbfrel() */
#define FTNUFFN      	63	/* ftufulfn() */

	/* search requests */
#define FTNSHSTRT	22	/* ftshstrt() */
#define FTNSCLOSE	23	/* ftsclose() */
#define FTNSRNNCD	24	/* ftsrnncd() */
#define FTNSGEV		25	/* ftsgev() */

#define FTNSRLINK	26	/* ftsrlink() */
#define FTNSRCLSE	27	/* ftsrclse() */
#define FTNSROPEN	28	/* ftsropen() */
#define FTNSGIDX	57	/* get search result index object */

#define FTNXDOPN	29	/* ftfxdopn() */
#define FTNXLGET	30	/* ftfxlget() */
#define FTNXVPOS	31	/* ftfxvpos() */
#define FTNXCLSE	32	/* ftfxclse() */

	/* collection management */
#define FTNCLIST	33	/* ftuclist() */
#define FTNBCREAT	34	/* ftbcreate() */
#define FTNBREMOV	35	/* ftbremove() */
#define FTNBISTRT	36	/* ftbistrt() */
#define FTNFBLOG	37	/* ftbfblog() */

#define FTNSHCFLD	38	/* ftshcfld() */

	/* Query filter processing */
#define	FTNQHSTRT	59	/* ftqhstrt() */

	/* asynchronous requests */
#define FTNSACAN	41	/* search cancel */
#define FTNIACAN	42	/* indexing cancel */
#define FTNSAKIL	48	/* search kill (dispose of FTSB) */
#define	FTNQACAN	60	/* query filter processing cancel */

	/* Catalog stream mode requests */
#define FTNCFGET	43	/* ftcfget() */
#define FTNCFPUT	44	/* ftcfput() */
#define FTNCSFRE	45	/* ftcsfree() */
#define FTNCSRBL	46	/* ftcsrblk() */
#define	FTNCSRWR	47	/* ftncsrwr() */

	/* index browse requests */
#define	FTNXOPEN	50	/* ftxopen() */
#define	FTNXGETW	51	/* ftxgetw() */
#define	FTNXGEZ		52	/* ftxgez() */
#define	FTNXTELL	53	/* ftxtell() */
#define	FTNXSEEK	54	/* ftnxseek() */
#define	FTNXCLOSE	55	/* ftnxclose() */
#define	FTNXSTAT	56	/* ftnxstat() */

	/* session file requests */
#define FTNSMOPEN	58	/* ftnsmopen() */
#define FTNSMCLSE	39	/* ftnsmclse() */
#define FTNSMKEEP	49	/* ftnsmkeep() */

	/* asynchronous server-to-client messages */
#define FTNSINFO	0
#define FTNIINFO	1
#define	FTNQINFO	2

	/* miscellaneous requests */
#define FTNENQ		0xff	/* test driver enquiry code */
#define FTNACK		0xfe	/* test driver acknowledge code */

	/* search response codes--these are sent from the server */
	/* to the client after ftsopen() */
#define FTNSQSEND	0xfd	/* send a bufferload of query */
#define FTNSNDOCS	0xfc	/* number of documents found so far */

	/* search cancel and continue requests; sent from client to */
	/* server after receipt of FTNSNDOCS message */
#define FTNSCAN		0xfb
#define FTNSCON		0xfa

	/* initial connection request; set by ftnconn() to server */
#define	FTNCONN		0xf9

	/* generic response codes */
#define FTNROK		0	/* function completed successfully */
#define FTNRERR		1	/* function failed; fterrno follows */

	/* this code used only by network filbuf */
#define FTNROKEOF	2	/* success; last bytes in response */

#endif /*!ftnconst_h*/
