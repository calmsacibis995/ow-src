/*static char *sccsid="@(#) ftnfb.h (1.3)  14:21:08 07/08/97";

   ftnfb.h -- type declarations for network filter layer

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------
  | StM 5.1B 91/06/20 | vcnfid[] size from FTGMXFILT to FTNMXFILT
  | swr 5.1B 91/05/27 | 91012901: added #ifdef FTCPP (C++ safe links)
  | hjb	5.0W 91/02/07 | FAR n_buf; n_ptr,n_epkt -> int; n_bufh; prototypes
  | hjb	5.0W 91/02/01 | add n_apih
  | bf  5.0D 90/08/21 | add vcrecno
  | bf  5.0D 90/08/10 | prefix FTNVCIRC element names with vc
  | bf  5.0D 90/08/07 | add FTNVCIRC
  | bf  5.0D 90/07/25 | add n_proto, n_flags; change n_data to long--alignment
  | bf  5.0C 90/03/16 | add n_list to FTNFILTT
  | bf  5.0A 89/12/05 | drop n_ebuf
  | bf  5.0  89/11/07 | include fthandle.h instead of ftgtypes.h
  | bf  4.6  89/02/09 | rename to ftnfb.h; split out ftnfuncs.h
  |-------------------|---------------------------------

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */

#ifndef	ftnfb_h
#define ftnfb_h	1

#ifndef	fthndl_h
#include "fthandle.h"	/* FTNFH */
#endif
#ifndef	ftgcon_h
#include "ftgconst.h"	/* FTGNODESZ, FTGMXFILT */
#endif

	/* network filter stream macros */
#define ftnfbcls(nfb)		((*(nfb)->n_close)(nfb))
#define ftnfbrd(nfb,b,l)	((*(nfb)->n_read)(nfb,b,l))
#define ftnfbwrt(nfb,b,l)	((*(nfb)->n_write)(nfb,b,l))

#ifdef	FTCPP
extern	"C" {
#endif
	/* ftnfb -- control block used by network filters */
typedef struct _FTNFB	FTNFB;
struct	_FTNFB {
					/* set of network i/o functions: */
	int		(*n_close)FTPL((FTNFB *));
	int		(*n_read)FTPL((FTNFB *, char FAR *, int));
	int		(*n_write)FTPL((FTNFB *, char FAR *, int));

	int		n_packsz;	/* optimal packet size */
	unsigned	n_flags;	/* flag bits */
	FTGMH		n_bufh;		/* handle for buffer if moveable */
	char	FAR	*n_buf;		/* ftnprntf/scanf buffer */
	int		n_ptr;		/* current buffer position */
	int		n_epkt;		/* end-of-packet for ftnscanf */
	FTNFH		n_next;		/* next filter in chain or NULL */
	USHORT		n_proto;	/* protocol version in use */
	FTAPIH		n_apih;		/* API handle */
	long		n_data[1];	/* private data kept by functions */
};

	/* ftnfiltt -- network filter table entry */
typedef struct ftnfiltt {
	char		*n_nfid;		/* network filter-id string */
	FTNFB *		(*n_open)FTPL((FTAPIH, char FAR *, char FAR *));
						/* client open */
	FTNFB *		(*n_serve)FTPL((FTAPIH, char FAR *));
						/* server open */
	int		(*n_find)
#ifdef ftbtyp_h
			    FTPL((FTAPIH, char FAR *, char FAR *, FTFIG FAR *));
#else /*!ftbtyp_h*/
			    ();
#endif /*!ftbtyp_h*/
						/* directory services: find */
	int		(*n_list)FTPL((FTAPIH, char FAR *, int (*)(), char *,
			    unsigned));		/* directory services: list */
} FTNFILTT;
#ifdef	FTCPP
}
#endif

	/* ftnvcirc - maintain a virtual circuit with use count */
typedef struct ftnvcirc {
	FTNFH	vcnfh;
	int	vcusecnt;
	char	vcrecno;
	char	vcnode[FTGNODESZ];
	char	vcnfid[FTNMXFILT];
} FTNVCIRC;

#include "ftnfuncs.h"

#endif	/*!ftnfb_h*/
