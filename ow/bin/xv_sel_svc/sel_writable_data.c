#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)sel_writable_data.c 20.1 88/04/14";
#endif
#endif

#include "seln_svc.h"
#include <xview/selection.h>

/*
 * sel_writable_data.c:	writable initialized data (must not go in text
 * segment)
 * 
 */

struct timeval  seln_std_timeout = {
    SELN_STD_TIMEOUT_SEC, SELN_STD_TIMEOUT_USEC
};

int             seln_svc_program = SELN_SVC_PROGRAM;
