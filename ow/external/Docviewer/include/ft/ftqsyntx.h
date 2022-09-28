/*static char *sccsid="@(#) ftqsyntx.h (1.1)  14:21:08 07/08/97";

   ftqsyntx.h - define escape sequence for query file syntax.

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------
  | PW  5.0  89/08/14 | add FTQICWGT
  |-------------------|---------------------------------

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */

#ifndef FTFCSI
#include "ftfctype.h"
#endif

/* special characters and character sequences	       */
#ifndef NUL
#define NUL    FTFNUL
#endif
#ifndef CSI
#define CSI    FTFCSI
#endif

#define CSIS   FTFESC,FTFESCSI
#define PCD    '%','d'	/* NCS */
#define PCC    '%','c'	/* NCS */


/* final character of escape sequence identifies the meta-character    */
#define FTQFCPHR       FTFCSPRV
#define FTQFCRES       0x72
#define FTQFCFDS       0x73
#define FTQFCAND       0x74
#define FTQFCOR        0x75
#define FTQFCENV       0x76
#define FTQFCWLD       0x77
#define FTQFCNOT       0x78
#define FTQFCPXY       0x79
#define FTQFCNEAR      0x79
#define FTQFCDAT       0x7a
#define FTQFCRNG       0x7a
#define FTQFCEND       0x7d
#define FTQFCHYP       FTFTILDE

#ifdef DATAMAT
#define FTQFCLEM       0x7c
#endif /*DATAMAT*/

/* intermediate character of escape sequence is a further differentiation*/
#define FTQICEXC       FTFIEXCL
#define FTQICDQ        FTFDQUOT
#define FTQICNUM       FTFPOUND
#define FTQICRTE       FTFASTER
#define FTQICXRF       FTFIPLUS
#define FTQICINX       FTFHYP
#define FTQICEXA       FTFPERIOD
#define FTQICAFT       FTFGT
#define FTQICBEF       FTFLT
#define FTQICEQU       FTFEQUAL
#define FTQICWGT       FTFDOLR	

  /* separator characters used in parameters found in control sequences */
#define QVPPSEP        FTFCSSEP
#define QVPVSEP        FTFCSCOL

  /* complete control sequences - static char array initializers */
#define FTQMCEXP       {CSIS, FTQICINX, FTQFCWLD, NUL}
#define FTQMCHYP       {CSIS, FTQICINX, FTQFCHYP, NUL}
#define FTQSDAFT       {CSIS, FTQICAFT, FTQFCDAT, NUL}
#define FTQSDBEF       {CSIS, FTQICBEF, FTQFCDAT, NUL}
#define FTQSVRNG       {CSIS, FTQICINX, FTQFCDAT, NUL}
#define FTQSBUT 	       {CSIS, FTQFCNOT, NUL}
#define FTQSAND 	       {CSIS, FTQFCAND, NUL}
#define FTQSOR	       {CSIS, FTQFCOR,	NUL}
#define FTQSPHR 	       {CSIS, FTQFCPHR, NUL}
#define FTQSNEAR       {CSIS, FTQFCPXY, NUL}
#define FTQSEND 	       {CSIS, FTQFCEND, NUL}
#define FTQSRANG       {CSIS, FTQICNUM, FTQFCRNG, NUL}
#define FTQSTHNM       {CSIS, FTQICDQ,	FTQFCENV, NUL}
#define FTQSLMNM       {CSIS, FTQICEXC,  FTQFCENV, NUL}

  /* control sequences suitable as control string of fprintf() */
#define FTQPPARM       {CSIS, PCD, NUL}
#define FTQPPHR 	       {CSIS, PCD, FTQFCPHR, NUL}
#define FTQPNEAR       {CSIS, PCD, FTQFCNEAR, NUL}
#define FTQPFDS 	       {CSIS, PCD, FTQFCFDS, NUL}
#define FTQPENV 	       {CSIS, PCD, QVPVSEP, PCD, FTQFCENV, NUL}
