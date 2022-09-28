#pragma ident   "@(#)iso2022_to_cns11643.c	1.4 96/01/10 SMI; ALE"

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
#define MSB_OFF	0x7f	/* mask off MBS */

#define SI	0x0f	/* shift in */
#define SO	0x0e	/* shift out */
#define ESC	0x1b	/* escape */

/*
 * static const char plane_char[] = "0GH23456789:;<=>?";
 * static const char plane_char[] = "0GHIJKLMNOPQRSTUV";
 * #define	GET_PLANEC(i)	(plane_char[i])
 */

#define NON_ID_CHAR '_'	/* non-identified character */

typedef struct _icv_state {
	char	keepc[4];	/* maximum # byte of CNS11643 code */
	short	cstate;		/* state machine id */
	int	plane_no;	/* plane number for Chinese character */
	int	_errno;		/* internal errno */
} _iconv_st;

enum _CSTATE	{ C0, C1, C2, C3, C4, C5, C6, C7 };

extern int errno;

int get_plane_no_by_iso(const char);


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
	st->plane_no = 0;
	st->_errno = 0;

#ifdef DEBUG
    fprintf(stderr, "==========    iconv(): ISO2022-7 --> CNS 11643    ==========\n");
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
/*=========================================================================
 *
 *             State Machine for interpreting ISO 2022-7 code
 *
 *=========================================================================
 *
 *                                                        plane 2 - 16
 *                                                    +---------->-------+
 *                                    plane           ^                  |
 *            ESC      $       )      number     SO   | plane 1          v
 *    +-> C0 ----> C1 ---> C2 ---> C3 ------> C4 --> C5 -------> C6     C7
 *    |   | ascii  | ascii | ascii |    ascii |   SI | |          |      |
 *    +----------------------------+    <-----+------+ +------<---+------+
 *    ^                                 |
 *    |              ascii              v
 *    +---------<-------------<---------+
 *
 *=========================================================================*/
size_t
_icv_iconv(_iconv_st *st, char **inbuf, size_t *inbytesleft,
				char **outbuf, size_t *outbytesleft)
{
	int		n;

	if (st == NULL) {
		errno = EBADF;
		return ((size_t) -1);
	}

	if (inbuf == NULL || *inbuf == NULL) { /* Reset request. */
		st->cstate = C0;
		st->_errno = 0;
		return ((size_t) 0); 
	}

#ifdef DEBUG
    fprintf(stderr, "=== (Re-entry)   iconv(): ISO 2022-7 --> CNS 11643   ===\n");
#endif
	st->_errno = 0;         /* reset internal errno */
	errno = 0;		/* reset external errno */

	/* a state machine for interpreting ISO 2022-7 code */
	while (*inbytesleft > 0 && *outbytesleft > 0) {
		switch (st->cstate) {
		case C0:		/* assuming ASCII in the beginning */
			if (**inbuf == ESC) {
				st->cstate = C1;
			} else {	/* real ASCII */
				**outbuf = **inbuf;
				(*outbuf)++;
				(*outbytesleft)--;
			}
			break;
		case C1:		/* got ESC, expecting $ */
			if (**inbuf == '$') {
				st->cstate = C2;
			} else {
				**outbuf = ESC;
				(*outbuf)++;
				(*outbytesleft)--;
				st->cstate = C0;
				st->_errno = 0;
				continue;	/* don't advance inbuf */
			}
			break;
		case C2:		/* got $, expecting ) */
			if ((**inbuf == ')') || (**inbuf == '*') ||
				(**inbuf == '+')) {
				st->cstate = C3;
			} else {
				if (*outbytesleft < 2) {
					st->_errno = errno = E2BIG;
					return(-1);
				}
				**outbuf = ESC;
				*(*outbuf+1) = '$';
				(*outbuf) += 2;
				(*outbytesleft) -= 2;
				st->cstate = C0;
				st->_errno = 0;
				continue;	/* don't advance inbuf */
			}
			break;
		case C3:		/* got ) expecting G,H,I,...,V */
			st->plane_no = get_plane_no_by_iso(**inbuf);
			if (st->plane_no > 0 ) {	/* plane #1 - #16 */
		    		st->cstate = C4;
			} else {
		    		if (*outbytesleft < 3) {
					st->_errno = errno = E2BIG;
					return(-1);
				}
		    		**outbuf = ESC;
		    		*(*outbuf+1) = '$';
				if (*(*inbuf-1) == ')')
		    		   *(*outbuf+2) = ')';
				else if (*(*inbuf-1) == '*')
				   	*(*outbuf+2) = '*';
				else
					*(*outbuf+2) = '+';
            	    		(*outbuf) += 3;
				(*outbytesleft) -= 3;
		    		st->cstate = C0;
				st->_errno = 0;
		    		continue;	/* don't advance inbuf */
			}
			break;
	    	case C4:		/* SI (Shift In) */
			if (**inbuf == ESC) {
                           if ((*(*inbuf+1) != 0x4E) && (*(*inbuf+1) != 0x4F)) {
				st->cstate = C1;
				break;
			   }
			}
			if (((**inbuf == SO) && (st->plane_no == 1)) || 
			    ((st->plane_no >= 2) && (**inbuf == 0x1B) && 
			     (*(*inbuf+1) == 0x4E || *(*inbuf+1) == 0x4F))) {
#ifdef DEBUG
    fprintf(stderr, "<--------------  SO  -------------->\n");
#endif
				if (st->plane_no >= 2) {
				        (*inbuf)++;
                                        (*inbytesleft)--;
				}

		    		st->cstate = C5;
			} else {	/* ASCII */
		    		**outbuf = **inbuf;
		    		(*outbuf)++;
				(*outbytesleft)--;
				st->cstate = C0;
				st->_errno = 0;
			}
			break;
	    	case C5:		/* SO (Shift Out) */
			if ((**inbuf == SI) || (**inbuf == ESC) || 
			    (*(*inbuf-4) == 0x1B) && 
		            ((*(*inbuf-3) == 0x4E) || (*(*inbuf-3) == 0x4F)) &&
			     (**inbuf != 0x1B)) /* ASCII */ {
#ifdef DEBUG
    fprintf(stderr, ">--------------  SI  --------------<\n");
#endif
		    		st->cstate = C4;
				if (**inbuf != SI) {
					(*inbuf)--;
					(*inbytesleft)++;
				}
			} else {	/* 1st Chinese character */
					
				if (st->plane_no == 1) {
		    			st->keepc[0] = (char) (**inbuf | MSB);
		    			st->cstate = C6;
				} else {	/* 4-bypte code: plane #2 - #16 */
                                	if ((**inbuf == 0x1B) && 
					    (*(*inbuf+1) == 0x4E) ||
                                    	    (**inbuf == 0x1B) &&
                                            (*(*inbuf+1) == 0x4F)) {
					(*inbuf) += 2;
					(*inbytesleft) -= 2;
                                	}
					st->keepc[0] = (char) MBYTE;
					st->keepc[1] = (char) (PMASK + 
								st->plane_no);

		    			st->keepc[2] = (char) (**inbuf | MSB);
					st->cstate = C7;
				}
			}
			break;
	    	case C6:		/* plane #1: 2nd Chinese character */
			st->keepc[1] = (char) (**inbuf | MSB);
			st->keepc[2] = st->keepc[3] = NULL;
			n = iso_to_cns(1, st->keepc, *outbuf, *outbytesleft);
			if (n > 0) {
				(*outbuf) += n;
				(*outbytesleft) -= n;
			} else {
				st->_errno = errno;
				return(-1);
			}
			st->cstate = C5;
			break;
	    	case C7:		/* 4th Chinese character */
			st->keepc[3] = (char) (**inbuf | MSB);
			n = iso_to_cns(st->plane_no, st->keepc, *outbuf,
					*outbytesleft);
			if (n > 0) {
				(*outbuf) += n;
				(*outbytesleft) -= n;
			} else {
				st->_errno = errno;
				return(-1);
			}
			st->cstate = C5;
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
    fprintf(stderr, "!!!!!\tst->_errno = %d\tst->cstate = %d\tinbuf=%x\n",
		st->_errno, st->cstate, **inbuf);
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
 * Get plane number by ISO plane char; i.e. 'G' returns 1, 'H' returns 2, etc.
 * Returns -1 on error conditions
 */
int get_plane_no_by_iso(const char inbuf)
{
	int ret;
	unsigned char uc = (unsigned char) inbuf;

	if (uc == '0')	/* plane #0 */
		return(0);

	ret = uc - 'F';
	switch (ret) {
	case 1:		/* 0x8EA1 - G */
	case 2:		/* 0x8EA2 - H */
	case 3:		/* 0x8EA3 - I */
	case 4:		/* 0x8EA4 - J */
	case 5:		/* 0x8EA5 - K */
	case 6:		/* 0x8EA6 - L */
	case 7:		/* 0x8EA7 - M */
	case 8:		/* 0x8EA8 - N */
	case 9:		/* 0x8EA9 - O */
	case 10:	/* 0x8EAA - P */
	case 11:	/* 0x8EAB - Q */
	case 12:	/* 0x8EAC - R */
	case 13:	/* 0x8EAD - S */
	case 14:	/* 0x8EAE - T */
	case 15:	/* 0x8EAF - U */
	case 16:	/* 0x8EB0 - V */
		return (ret);
	default:
		return (-1);
	}
}


/*
 * ISO 2022-7 code --> CNS 11643-1992 (Chinese EUC)
 * Return: > 0 - converted with enough space in output buffer
 *         = 0 - no space in outbuf
 */
int iso_to_cns(plane_no, keepc, buf, buflen)
int	plane_no;
char	keepc[];
char	*buf;
size_t	buflen;
{
	int             ret_size;       /* return buffer size */

#ifdef DEBUG
    fprintf(stderr, "%s %d ", keepc, plane_no);
#endif
	if (plane_no == 1)
		ret_size = 2;
	else
		ret_size = 4;

        if (buflen < ret_size) {
                errno = E2BIG;
                return(0);
        }

	switch (plane_no) {
	case 1:
		*buf = keepc[0];
		*(buf+1) = keepc[1];
		break;
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
		*buf = keepc[0];
		*(buf+1) = keepc[1];
		*(buf+2) = keepc[2];
		*(buf+3) = keepc[3];
		break;
	}

#ifdef DEBUG
    fprintf(stderr, "\t#%d ->%s<-\n", plane_no, keepc);
#endif

	return(ret_size);
}
