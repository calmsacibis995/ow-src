#pragma ident   "@(#)cns11643_to_iso2022.c	1.5 96/01/10 SMI; ALE"

/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include <stdio.h>
#include <errno.h>

#define	MSB	0x80	/* most significant bit */
#define	MBYTE	0x8e	/* multi-byte (4 byte character) */
#define	PMASK	0xa0	/* plane number mask */
#define ONEBYTE	0xff	/* right most byte */
#define MSB_OFF	0x7f	/* mask off MSB */

#define SI      0x0f    /* shift in */
#define SO      0x0e    /* shift out */
#define ESC     0x1b    /* escape */

/* static const char plane_char[] = "0GH23456789:;<=>?"; */
static const char plane_char[] = "0GHIJKLMNOPQRSTUV";

#define GET_PLANEC(i)   (plane_char[i])

#define NON_ID_CHAR '_'	/* non-identified character */

typedef struct _icv_state {
	char	keepc[4];	/* maximum # byte of CNS11643 code */
	short	cstate;		/* state machine id (CNS) */
	short	istate;		/* state machine id (ISO) */
	int	_errno;		/* internal errno */
} _iconv_st;

enum _CSTATE	{ C0, C1, C2, C3, C4 };
enum _ISTATE    { IN, OUT };

extern int errno;

int get_plane_no_by_char(const char);


/*
 * Open; called from iconv_open()
 */
void *
_icv_open()
{
	_iconv_st *st;

	if ((st = (_iconv_st *)malloc(sizeof(_iconv_st))) == NULL) {
		errno = ENOMEM;
		return ((void *) -1);
	}

	st->cstate = C0;
	st->istate = IN;
	st->_errno = 0;

#ifdef DEBUG
    fprintf(stderr, "==========     iconv(): CNS11643 --> ISO 2022-CN     ==========\n");
#endif

	return ((void *) st);
}


/*
 * Close; called from iconv_close()
 */
void
_icv_close(_iconv_st *st)
{
	if (!st)
		errno = EBADF;
	else
		free(st);
}


/*
 * Actual conversion; called from iconv()
 */
/*=======================================================
 *
 *   State Machine for interpreting CNS 11643 code
 *
 *=======================================================
 *
 *               (ESC,SO)   plane 2 - 16
 *                1st C         2nd C       3rd C
 *    +------> C0 -----> C1 -----------> C2 -----> C3
 *    |  ascii |  plane 1 |                   4th C |
 *    ^        |  2nd C   v                         v
 *    |        |         C4 <------<--------<-------+
 *    |        v          | (SI)
 *    +----<---+-----<----v
 *
 *=======================================================*/
size_t
_icv_iconv(_iconv_st *st, char **inbuf, size_t *inbytesleft,
				char **outbuf, size_t *outbytesleft)
{
	int		plane_no, n;
	/* pre_plane_no: need to be static when re-entry occurs on errno set */
	static int	pre_plane_no = -1;	/* previous plane number */

	if (st == NULL) {
		errno = EBADF;
		return ((size_t) -1);
	}

	if (inbuf == NULL || *inbuf == NULL) { /* Reset request. */
		if (st->cstate == C1) {
			if (outbytesleft && *outbytesleft >= 1
				&& outbuf && *outbuf) {
				**outbuf = SI;
				(*outbuf)++;
				(*outbytesleft)--;
			} else {
				errno = E2BIG;
				return((size_t) -1);
			}
		}
		st->cstate = C0;
		st->istate = IN;
		st->_errno = 0;
		return ((size_t) 0); 
	}

#ifdef DEBUG
    fprintf(stderr, "=== (Re-entry)     iconv(): CNS11643 --> ISO 2022-CN     ===\n");
    fprintf(stderr, "st->cstate=%d\tst->istate=%d\tst->_errno=%d\tplane_no=%d\n",
	st->cstate, st->istate, st->_errno, plane_no);
#endif
	st->_errno = 0;         /* reset internal errno */
	errno = 0;		/* reset external errno */

	/* a state machine for interpreting CNS 11643 code */
	while (*inbytesleft > 0 && *outbytesleft > 0) {
		switch (st->cstate) {
		case C0:		/* assuming ASCII in the beginning */
			if (**inbuf & MSB) {
				st->keepc[0] = (**inbuf);
				st->cstate = C1;
			} else {	/* real ASCII */
				if (st->istate == OUT) {
					st->cstate = C0;
					st->istate = IN;
					if (plane_no == 1) {
						**outbuf = SI;
						(*outbuf)++;
						(*outbytesleft)--;
						if (*outbytesleft <= 0) {
							errno = E2BIG;
							return(-1);
						}
					}
				}
				**outbuf = **inbuf;
				(*outbuf)++;
				(*outbytesleft)--;
			}
			break;
		case C1:		/* Chinese characters: 2nd byte */
			if ((st->keepc[0] & ONEBYTE) == MBYTE) { /* 4-byte (0x8e) */
				plane_no = get_plane_no_by_char(**inbuf);
				if (plane_no == -1) {	/* illegal plane */
					st->cstate = C0;
					st->istate = IN;
					st->_errno = errno = EILSEQ;
				} else {	/* 4-byte Chinese character */
					st->keepc[1] = (**inbuf);
					st->cstate = C2;
				}
			} else {	/* 2-byte Chinese character - plane #1 */
				if (**inbuf & MSB) {	/* plane #1 */
					st->cstate = C4;
					st->keepc[1] = (**inbuf);
					st->keepc[2] = st->keepc[3] = NULL;
					plane_no = 1;
					continue;       /* should not advance *inbuf */
				} else {	/* input char doesn't belong
						 * to the input code set 
						 */
					st->cstate = C0;
					st->istate = IN;
					st->_errno = errno = EINVAL;
				}
			}
			break;
		case C2:	/* plane #2 - #16 (4 bytes): get 3nd byte */
			if (**inbuf & MSB) {	/* 3rd byte */
				st->keepc[2] = (**inbuf);
				st->cstate = C3;
			} else {
				st->_errno = errno = EINVAL;
				st->cstate = C0;
			}
			break;
		case C3:	/* plane #2 - #16 (4 bytes): get 4th byte */
			if (**inbuf & MSB) {	/* 4th byte */
				st->cstate = C4;
				st->keepc[3] = (**inbuf);
				continue;       /* should not advance *inbuf */
			} else {
				st->_errno = errno = EINVAL;
				st->cstate = C0;
			}
			break;
		case C4:	/* Convert code from CNS 11643 to ISO 2022-CN */
			if ((st->istate == IN) || (pre_plane_no != plane_no)) {
				/* change plane # in Chinese mode */
				if (st->istate == OUT) {
					if (plane_no < 2)  {
						**outbuf = SI;
						(*outbuf)++;
						(*outbytesleft)--;
					}
#ifdef DEBUG
fprintf(stderr, "(plane #=%d\tpre_plane #=%d)\t", plane_no, pre_plane_no);
#endif
				}
				if (*outbytesleft < 4) {
					st->_errno = errno = E2BIG;
					return(-1);
				}
				pre_plane_no = plane_no;
				st->istate = OUT;	/* shift out */
				**outbuf = ESC;
				*(*outbuf+1) = '$';
				if ( plane_no == 1)
					 *(*outbuf+2) = ')';
				else if (plane_no == 2)
                                         *(*outbuf+2) = '*';
				else
					 *(*outbuf+2) = '+';		
                                *(*outbuf+3) = GET_PLANEC(plane_no);

#ifdef DEBUG
fprintf(stderr, "ESC $ %c %c\n", *(*outbuf+2), *(*outbuf+3));
#endif
				(*outbuf) += 4;
				(*outbytesleft) -= 4;
				if (*outbytesleft <= 0) {
					st->_errno = errno = E2BIG;
					return(-1);
				}
				if ( plane_no == 1) {
					**outbuf = SO;
					(*outbuf)++;
					(*outbytesleft)--;
				} else if ( plane_no == 2) {
					**outbuf = 0x1B;
                                        *(*outbuf+1) = 0x4E;
					(*outbuf) += 2;
					(*outbytesleft) -= 2;
				} else {
					**outbuf = 0x1B;
                                        *(*outbuf+1) = 0x4F;
                                        (*outbuf) += 2;
                                        (*outbytesleft) -= 2;
				}
			}
                        if ((plane_no == 2) && (*(*outbuf-4) == 0x1B)
					    && (*(*outbuf-3) == 0x4E)) {
                                        **outbuf = 0x1B;
                                        *(*outbuf+1) = 0x4E;
                                        (*outbuf) += 2;
                                        (*outbytesleft) -= 2;
			} else if ((plane_no > 2) && (*(*outbuf-4) == 0x1B)
			                         && (*(*outbuf-3) == 0x4F)) {
                                        **outbuf = 0x1B;
                                        *(*outbuf+1) = 0x4F;
                                        (*outbuf) += 2;
                                        (*outbytesleft) -= 2;
			}			
			n = cns_to_iso(plane_no, st->keepc, *outbuf, *outbytesleft);
			if (n > 0) {
				(*outbuf) += n;
				(*outbytesleft) -= n;
			} else {
				st->_errno = errno;
				return(-1);
			}
			st->cstate = C0;
			break;
		default:			/* should never come here */
			st->_errno = errno = EILSEQ;
			st->cstate = C0;	/* reset state */
			break;
		}

		(*inbuf)++;
		(*inbytesleft)--;

		if (st->_errno) {
#ifdef DEBUG
    fprintf(stderr, "!!!!!\tst->_errno = %d\tst->cstate = %d\n",
		st->_errno, st->cstate);
#endif
			break;
		}
		if (errno)
			return(-1);
	}

	if (*outbytesleft == 0) {
		errno = E2BIG;
		return(-1);
	}

	return (*inbytesleft);
}


/*
 * Get plane number by char; i.e. 0xa2 returns 2, 0xae returns 14, etc.
 * Returns -1 on error conditions
 */
int get_plane_no_by_char(const char inbuf)
{
	int ret;
	unsigned char uc = (unsigned char) inbuf;

	ret = uc - PMASK;
	switch (ret) {
	case 1:		/* 0x8EA1 */
	case 2:		/* 0x8EA2 */
	case 3:		/* 0x8EA3 */
	case 4:		/* 0x8EA4 */
	case 5:		/* 0x8EA5 */
	case 6:		/* 0x8EA6 */
	case 7:		/* 0x8EA7 */
	case 12:	/* 0x8EAC */
	case 14:	/* 0x8EAE */
	case 15:	/* 0x8EAF */
	case 16:	/* 0x8EB0 */
		return (ret);
	default:
		return (-1);
	}
}


/*
 * CNS 11643 code --> ISO 2022-7
 * Return: > 0 - converted with enough space in output buffer
 *         = 0 - no space in outbuf
 */
int cns_to_iso(plane_no, keepc, buf, buflen)
int	plane_no;
char	keepc[];
char	*buf;
size_t	buflen;
{
	char		cns_str[3];
	unsigned long	cns_val;	/* MSB mask off CNS 11643 value */

#ifdef DEBUG
    fprintf(stderr, "3333333333: (%x %x %x %x) (%d) \n", keepc[0],keepc[1],keepc[2],keepc[3], plane_no);
#endif

        if (buflen < 2) {
                errno = E2BIG;
                return(0);
        }

	if (plane_no == 1) {
		cns_str[0] = keepc[0] & MSB_OFF;
		cns_str[1] = keepc[1] & MSB_OFF;
	} else {
		cns_str[0] = keepc[2] & MSB_OFF;
		cns_str[1] = keepc[3] & MSB_OFF;
	}
	cns_val = (cns_str[0] << 8) + cns_str[1];
#ifdef DEBUG
    fprintf(stderr, "4444444444:%x\t", cns_val);
#endif

	*buf = (cns_val & 0xff00) >> 8;
	*(buf+1) = cns_val & 0xff;

#ifdef DEBUG
    fprintf(stderr, "->%x %x<-\t->%c %c<-\n", *buf, *(buf+1), *buf, *(buf+1));
#endif

	return(2);
}
