#ifndef lint
        static char sccsid[] = "@(#)iserror.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * iserror.c
 *
 * Description:
 *   	NetISAM error handling functions.
 *
 */

#include "isam_impl.h"
#include <syslog.h>

/*
 * _isfatal_error(msg)
 *
 * Fatal error. Display message and terminate program.
 */

static int (*fatal_error_user_handler)();    /* set by iscntl(..,ISCNTL_FATAL,..) */

void 
_isfatal_error(msg)
    char	*msg;
{
  extern int	errno;
  int		logerr;

  if (fatal_error_user_handler) {
    logerr = fatal_error_user_handler(msg); /* User returns 1 in order
					     * to use syslog() 
					     */
  }
  else
    logerr = 1;

  if (logerr) {
    openlog("NetISAM", LOG_PID, LOG_USER);

    /* Free one UNIX for syslog */
    (void)close(0);			    
					    
    syslog(LOG_ERR, "Fatal error: %s - UNIX errno %d", msg, errno);

    closelog();
  }
  exit (1);
}

void 
_isfatal_error1(msg)
    char	*msg;
{
  extern int	errno;
  extern int	_is_rpcnetisamd; /* is 1 if this is rpc.netisamd */
  extern int	_is_netisamlockd; /* is 1 if this is netisamlockd */
  int		logerr;

  if (fatal_error_user_handler) {
    logerr = fatal_error_user_handler(msg); /* User returns 1 in order
					     * to use syslog() 
					     */
  }
  else
    logerr = 1;

  if (logerr) {
      openlog("NetISAM", LOG_PID, LOG_USER);

    /* Free one UNIX for syslog */
    (void)close(0);			    
					    
    syslog(LOG_ERR, "Fatal error: %s - UNIX errno %d", msg, errno);

    closelog();
  }
}

_isam_warning(msg)
    char	*msg;
{
    openlog("NetISAM", LOG_PID, LOG_USER);
    syslog(LOG_ERR, msg);
}

/* Set user specified fatal_error handler */
int  (*_isfatal_error_set_func(func))()
    int		(*func)();
{
    int		(*oldfunc)();

    oldfunc = fatal_error_user_handler;
    fatal_error_user_handler = func;

    return (oldfunc);
}

/*
 * _setiserrno2(error, isstat1, isstat2)
 *
 * Set iserrno variable.
 */

void
_setiserrno2(error, is1, is2)
    int		error;
    int		is1, is2;
{
    iserrno = error;
    isstat1 = is1;
    isstat2 = is2;
}

/*
 * _seterr_errcode(errcode)
 *
 * Set all error and status variable from errcode structure.
 */

void
_seterr_errcode(errcode)
    register struct errcode	*errcode;
{
    iserrno = errcode->iserrno;
    isstat1 = errcode->isstat[0];
    isstat2 = errcode->isstat[1];
    isstat3 = errcode->isstat[2];
    isstat4 = errcode->isstat[3];
}
