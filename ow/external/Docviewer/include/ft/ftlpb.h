/*static char *sccsid="@(#) ftlpb.h (1.1)  14:21:07 07/08/97";			

   ftlpb.h  - typedefs for external linguistic packages					

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------		
  | swr 5.1B 91/05/27 | 91012901: added #ifdef FTCPP (C++ safe links)
  | wtr 5.1  91/05/17 | move private definitions to ftlpbdrv.h
  | wtr 5.1  91/05/02 | add ftlpungc macro
  | wtr 5.1  91/03/04 | created
  |-------------------|---------------------------------		

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */

#ifndef ftlpb_h
#define ftlpb_h


	/* FTLPB structure for management of ELP */
#ifdef	FTCPP
extern	"C" {
#endif
typedef struct _FTLPB	FTLPB;
struct  _FTLPB {
	FTAPIH		ftlpb_apih;	/* api instance handle */
	UCHAR FTBASED((_segment)_self)	*ftlpb_ptr;	/* current char */
	UCHAR FTBASED((_segment)_self)	*ftlpb_base;	/* buffer address */
	char FTBASED((_segment)_self)	*ftlpb_appl;	/* private data */

		/* pointers to the filter functions */
	int (FTLOADDS PASCAL	*ftlpb_close) FTPL((FTLPB FAR *));
	int (FTLOADDS PASCAL	*ftlpb_fbuf) FTPL((FTLPB FAR *));
	int (FTLOADDS PASCAL	*ftlpb_stat) FTPL((FTLPB FAR *, long, long *));
	int (FTLOADDS PASCAL	*ftlpb_pmes) FTPL((FTLPB FAR *, int, char *));
	int (FTLOADDS PASCAL	*ftlpb_slct) FTPL((FTLPB FAR *, int, int));

	short		ftlpb_cnt;	/* bytes remaining in buffer */
	short		ftlpb_bsiz;	/* buffer length */
};
#ifdef	FTCPP
}
#endif

/* macro for getting next character from a buffer */
#define ftlpgetint(p)       ((int)(*(p)++))

/* macro for returning a character to a buffer */
#define ftlpugtint(p,c)  (0xff&(*--(p)=(c)))
     
/* macro for getting a character from the ELP */
#define ftlpgetc(p) (--(p)->ftlpb_cnt>=0 ? ftlpgetint((p)->ftlpb_ptr) \
					: (*(p)->ftlpb_fbuf)(p))

/* macro for ungetting a character to the ELP */
#define ftlpungc(c,p) (++(p)->ftlpb_cnt, ftlpugtint((p)->ftlpb_ptr,(c)))

#define FTLPINSZ 3	/* size of the info array filled by the info function */
#define FTLPFLAG 0	/* index in info array to flag word */
#define FTLPIFBF 1	/* index in info array to size of char buffer */
#define FTLPIPVT 2	/* index in info array to size of private data area */

/* query id code defining the nature of a command to the query function */
# define FTLPLKUP 0	/* look up a word */
# define FTLPLULT 1	/* look up a literal */

/* selection id codes (sid's) for the ELP select function */
#define FTLPSALL 0	/* select all inflected and run-on forms */
#define FTLPIFLT 1      /* select all inflected forms */
#define FTLPRUN  2      /* select all run-on forms */
#define FTLPSTEM 3      /* select the stem word */
#define FTLPCATY 4      /* select a particular category */
#define FTLPALLC 5      /* select all categories of words*/
#define FTLPMEAN 6      /* send meaning associated with a particular sense */
#define FTLPWHAT 7      /* make list telling what categories are available */
#define FTLPCSLS 8      /* send selected list of words */

/* status id codes for telling ELP status function what status info to send */
#define FTLSANSEN 0	/* number of senses associaed with the last lookup */
#define FTLSANWRD 1	/* number of words in the current selection */
#define FTLSASTAT 2	/* the status of the ELP */

/* indices to longs array containing status returned by ELP status function */
#define FTLSNSEN 0	/* number of senses associaed with the last lookup */
#define FTLSNWRD 1	/* number of words in the current selection */
#define FTLSSTAT 2	/* the status of the ELP */

#define FTLSSIZ 3       /* number of elements in longs status array */

/* status codes that may be contained in the FTLSSTAT word of the status array*/
#define FTLSSSND 0	/* a request has been made to the ELP to send a list 
			   of words or a meaning */
#define FTLSSLKT 1	/* a lookup has been requested & resulted in success */
#define FTLSSLKF 2	/* a lookup has been requested & resulted in failure */

#define FTEPVARSEP 7     /* separator between variants coming from an ELP */

#endif  /* ftlpb_h */
