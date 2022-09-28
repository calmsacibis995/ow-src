
/*	@(#)seln_svc.h 1.7 92/02/26 */

#ifndef	_view2_utility_svc_DEFINED
#define	_view2_utility_svc_DEFINED

/*
 *	Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <xview/sun.h>
#include <xview_private/seln_impl.h>

extern int          errno;

#define SELN_SVC_PROGRAM    100015	/* SELNSVCPROG in <rpcsvc/??.h>	*/
#define SELN_LIB_PROGRAM    100025	/* SELNLIBPROG in <rpcsvc/??.h>	*/
#define SELN_SVC_TEST	0x20000000	/* flag ORed into program number  */
#define SELN_VERSION             6	/*  seln_get_function_state	  */
#define SELN_VERSION_ORIGINAL    1

/* Procedure IDs for server-module procedures	 */

#define SELN_SVC_NULLPROC	 0
#define SELN_SVC_ACQUIRE	 1
#define SELN_SVC_CLEAR_FUNCTIONS 2
#define SELN_SVC_DONE		 3
#define SELN_SVC_GET_HOLDER	 4
#define SELN_SVC_HOLD_FILE	 5
#define SELN_SVC_INFORM		 6
#define SELN_SVC_INQUIRE	 7
#define SELN_SVC_INQUIRE_ALL	 8
#define SELN_SVC_STOP		 9
#define SELN_SVC_FUNCTION_ONE	10
#define SELN_SVC_FUNCTIONS_ALL	11



/* Random lengths and limits	 */

#define SELN_HOSTNAME_LENGTH   256	/* see socket(2)  */
#define SELN_MAX_PATHNAME     1024	/* MAXPATHLEN in <sys/param.h>
					 * (which includes a whole bunch						 * of other stuff I've already got)
					 */

#define NO_PMAP	0	/* svc_register skips its pmap_set if protocol == 0  */

/*	functions for dumping & tracing; in debug.c	*/

void	seln_dump_file_args(),
	seln_dump_function_buffer(),
	seln_dump_holder(),
	seln_dump_inform_args(),
	seln_dump_rank(),
	seln_dump_result(),
	seln_dump_state();

Seln_result
	seln_dump_service();

/*	routines to translate date in rpc			*/

int	xdr_seln_access(),
	xdr_seln_holder(),
	xdr_seln_holders_all(),
	xdr_seln_file_info(),
	xdr_seln_function(),
	xdr_seln_functions_state(),
	xdr_seln_inform_args(),
	xdr_seln_reply(),
	xdr_seln_request(),
	xdr_sockaddr_in();

/*	common data defined in sel_common.c	*/

/*
extern Seln_access	 seln_my_access;
extern int		 seln_my_tcp_socket;
extern int		 seln_my_udp_socket;
extern SVCXPRT		*seln_my_tcp_transport;
extern SVCXPRT		*seln_my_udp_transport;
extern SVCXPRT		*seln_req_transport;
*/

extern struct timeval	 seln_std_timeout;
extern int               seln_svc_program;

extern Seln_result  	seln_convert_request_to_property();
#endif
