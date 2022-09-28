/*static char *sccsid="@(#) ftftypes.h (1.3)  14:21:05 07/08/97";

   ftftypes.h  - include file for filters

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------
  | ded 5.1W 91/05/17 | 91051501: ftfputc() - And 0xff with c 
  | bf  5.0W 91/01/10 | uncomment ftfedit define
  | hjb	5.0W 90/12/19 | moved ftfopen(),ftfsopen(),etc. (MACROS) to ftffuncs.h
  | bf  5.0W 90/10/30 | moved constants to ftfconst.h; dropped ftfedit,ftftsize
  | AH	5.0A 89/12/29 | Added FTF_CACHE
  | DG	5.0  89/10/05 | ftffpeek()
  | DG	4.6  88/12/22 | move translation tables symbols to pi/fttrtget.c
  |-------------------|---------------------------------

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */


#ifndef ftfb_h
#include "ftfb.h"
#endif
#include "ftfconst.h"

#ifdef	FTHUCHAR
#define ftfctint(c)	((int)(c))
#define ftfpctint(p)	((int)(*(p)))
#define ftfgetint(p)	((int)(*(p)++))
#define ftfugtint(p,c)	((int)(*--(p)=(c)))

	/* compiler bug in some environments requires special version */
	/* of ftfputint macro */
#ifdef	mc68k	/* mightyframe */
#define	FTPUTBUG
#endif
#ifdef	m68k	/* targon 31 */
#define	FTPUTBUG
#endif
#ifdef	MOT2000	/* Motorola 2000 */
#define	FTPUTBUG
#endif
#ifdef	FTPUTBUG
#define ftfputint(p,c)	(*(p)++=(c))
#else
#define ftfputint(p,c)	((int)(*(p)++=(c)))
#endif

#else	/*!FTHUCHAR*/
#define ftfctint(c)	((c)&0xff)
#define ftfpctint(p)	(*(p)&0xff)
#define ftfgetint(p)	(*(p)++&0xff)
#define ftfugtint(p,c)	(0xff&(*--(p)=(c)))
#define ftfputint(p,c)	(0xff&(*(p)++=(c)))
#endif	/*!FTHUCHAR*/

#define ftfgetc(p)	(--(p)->ftfb_cnt>=0 ? ftfgetint((p)->ftfb_ptr) :\
			    (*(p)->ftfb_fbuf)(p))

#define ftfungetc(c,p)	((p)->ftfb_flag |= FTF_IOUGC, ++(p)->ftfb_cnt,\
			    ftfugtint((p)->ftfb_ptr,(c)))

#define ftfputc(c,p)	(((p)->ftfb_ptr<(p)->ftfb_base+(p)->ftfb_bsiz)?\
			    ((p)->ftfb_flag |= FTF_UPD, (p)->ftfb_cnt--, \
			    ftfputint((p)->ftfb_ptr,(unsigned)(c))): \
			    (*(p)->ftfb_pbuf)((unsigned)(c & 0xff),p))

#define ftfpeek(p)	((p)->ftfb_cnt>0 ? (int)(*(p)->ftfb_ptr&0377) :\
			    ftfpbuf(p))

#define ftfclerr(p)	((p)->ftfb_flag &= ~FTF_IOEOF & ~FTF_IOERR)
#define ftfeof(p)	((p)->ftfb_flag & FTF_IOEOF)
#define ftferror(p)	((p)->ftfb_flag & FTF_IOERR)
#define ftfileno(p)	(ftfrtofd(ftfrfd(p)))
#define ftfrfd(p)	((int)((p)->ftfb_file))

	/* macro to return default editor name */
#define ftfedit(p)	((p)->ftfb_edit)
