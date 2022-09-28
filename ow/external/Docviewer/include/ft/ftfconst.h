/*static char *sccsid="@(#) ftfconst.h (1.1)  14:21:04 07/08/97";

   ftfconst.h  - constants for document text filters

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------
  | wtr 5.1A 91/05/15 | remove CONST FAR from static declaration
  | wtr 5.1A 91/05/08 | add FTFOLONG & longs vector definition macros
  | rtk 5.1A 91/04/30 | add FTFHFLAG, FTFIFLAG, FTFOFLAG
  | rtk 5.1A 91/04/29 | moved  FTFOFLG from ftgconst.h
  | ah 	5.1W 91/04/22 | 91032003: Renamed FTF_CACHE to FTF_CAT
  | wr  5.1  91/02/12 | Added FTFLWILD
  | bf  5.0W 90/12/22 | FTFMLOWR
  | bf  5.0W 90/10/30 | Created.
  |-------------------|---------------------------------

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */


#define FTFBDBSZ	255 	/* default buffer size */

	/* constants for ftfhop() longs vector */
#define FTFHFLAG	0	/* offset to flags word of longs array	*/
#define FTFOFLG		FTFHFLAG	/* for backwards compatibility	*/
#define FTFHMODE	1	/* offset to mode value of longs array	*/

	/* flag bits for FTFHFLAG */
#define FTFORVRS 	0x0001	/* dynamic table should be scanned first */

	/* mutually exclusive mode values for FTFHMODE */
#define FTFHNONE	0	/* no special mode or doc. display	*/
#define FTFHINDX	1	/* filter is being invoked for indexing */
#define FTFHDVEC	2	/* filter is being invoked for document */
				/* vector generation (reserved for future use)*/
#define FTFHSTAT	3	/* filter invoked for document validation */

	/* constants for open function's longs vector */
#define FTFOFLAG	0	/* offset to flags word of longs array	*/
#define FTFOMODE	1	/* offset fo mode value			*/
				/* mode values are as defined for FTFHMODE   */
	/* maximum number of longs */
#define	FTFOLONG	2


/* The following macros statically define longs vectors for ftfhop. */

/* longs vector for a filter is being invoked for indexing */
#define FTFOLVIX(name) static long name[] = {0, FTFHINDX}
/* longs vector for a filter is being invoked for document validation */
#define FTFOLVST(name) static long name[] = {0, FTFHSTAT}


	/* constants for info function's longs vector */
#define FTFMFLAG	0	/* flags */
#define FTFMBUFS	1	/* buffer size */
#define FTFMFTFT	2	/* number of FTFTs */
#define FTFMAPPL	3	/* bytes of application-specific data */
#define FTFMEDIT	4	/* bytes required for editor name */
#define FTFMLOWR	5	/* number of FTFTs required by lower filters */
#define FTFMVERS	6	/* version number of filter interface */
#define FTFMMODE	7	/* mode indicates reason for invoking filter */
				/* mode values are as defined for FTFHMODE   */
	/* maximum number of longs */
#define	FTFMLONG	8

	/* flag bits for FTFMFLAG */
#define FTFMISLO	0x0001	/* no lower-level filter required */
#define FTFMSING	0x0002	/* only a single state vector required */
#define FTFMINDX	0x0004	/* filter is being invoked for indexing */
#define FTFMDVEC	0x0008	/* filter is being invoked for document */
				/* vector generation */
#define FTFMSTAT	0x0010	/* filter invoked for document validation */
#define FTFMFALL	0x001F	/* all flag values OR'd together */

	/* version numbers for FTFMVERS */
#define FTFMV50W	1	/* new interface as of 5.0W */


	/* flags used in ftfb_flag */
	/* opened for read */
#define FTF_IOR 	0x0001
	/* ungetc has been performed in current buffer (must re-read if seek) */
#define FTF_IOUGC	0x0002
	/* must re-seek for every read (used by ftisrt) */
#define FTF_IOSK	0x0004
#define FTF_IOVE	0x0008
	/* at EOF */
#define FTF_IOEOF	0x0010
	/* error encountered */
#define FTF_IOERR	0x0020
	/* opened for write */
#define FTF_IOW 	0x0040
	/* buffer updated */
#define FTF_UPD 	0x0080
	/* buffer supplied by caller, must not be freed */
#define FTF_CBF 	0x0100
	/* file is a library file */
#define FTF_ISLIB	0x0200
	/* file is located on another node */
#define FTF_RMT 	0x0400
	/* current buffer is last in file */
#define FTF_LAST	0x0800
	/* ftfb is associated with a catalog stream */
	/* The catalog code will handle all aspects of the */
	/* close functions */
#define FTF_CAT		0x1000
	/* FTFB may be freed; normally set for top filter only */
#define FTF_ALLOC	0x2000

	/* separator char used between filters in the filter list */
#define FTFLSEP ':'
	/* separator between lib expansion filter list and model filter list*/
#define FTFLBSEP '!'
	/* marker for location info of a library's model filter list */
#define FTFLLOC  '@'
	/* separator char used to separate parameters of a filter */
#define FTFPSEP  '/'
        /* if a filter name is being compared with name in the the
           filter table, and this character is encountered in the name from
           the filter table, then all characters in the filter name from that
           point to the end of the string are deemed to match */
#define FTFLWILD  '*'
