#ifndef lint
static  char sccsid[] = "@(#)agent.c 3.6 92/10/06 Copyr 1991 Sun Microsystems, Inc.";
#endif

/*
 *  Copyright (c) 1987-1990 Sun Microsystems, Inc.
 *  All Rights Reserved.
 *
 *  Sun considers its source code as an unpublished, proprietary
 *  trade secret, and it is available only under strict license
 *  provisions.  This copyright notice is placed here only to protect
 *  Sun in the event the source is deemed a published work.  Dissassembly,
 *  decompilation, or other means of reducing the object code to human
 *  readable form is prohibited by the license agreement under which
 *  this code is provided to the user or company in possession of this
 *  copy.
 *
 *  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *  Government is subject to restrictions as set forth in subparagraph
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *  NASA FAR Supplement.
 */ 

#include <stdio.h>
#include <signal.h>
#include <sys/errno.h>
#include <rpc/rpc.h>

#ifdef SVR4
#include <netconfig.h>
#include <netdir.h>
#else
#include <sys/socket.h>
#endif "SVR4"

#include "util.h"
#include "rtable4.h"
#include "agent.h"
#include "abbcache.h"

extern	int	debug;
u_long		transient=0;

/*
 * get transient program number for callbacks.
 */
#ifdef SVR4
u_long
gettransient (version)
	u_long version; 
{
	int stat;
	struct nd_hostserv host = {HOST_SELF, "rpcbind"};
	struct nd_addrlist *addrp;
	static u_long prognum = 0x40000000;

        struct netbuf    *addr;
	struct netconfig *netconf;

	netconf = getnetconfigent("udp");
	if (!netconf) {
	    fprintf(stderr, "gettransient: getnetconfigent(%s) failed\n", "udp");
	    return 0;
	}

	stat = netdir_getbyname(netconf, &host, &addrp);
	if (stat) {
	    fprintf(stderr, "gettransient: netdir_getbyname failed\n");
	    return 0;
	}

 	if (addrp->n_cnt < 1) {
	    fprintf(stderr, "gettransient: netdir_getbyname - zero addresses\n");
	    return 0;
	}

        addr = addrp->n_addrs;

	while (!rpcb_set(prognum++, version, netconf, addr))
	    continue;

	prognum--;
	return prognum;
}

#else
u_long
gettransient (proto, vers, sockp)
	int proto; u_long vers; int *sockp;
{
	static u_long prognum = 0x40000000;
	int s, len, socktype;
	struct sockaddr_in addr;

	switch (proto) {
		case IPPROTO_UDP:
			socktype = SOCK_DGRAM;
			break;
		case IPPROTO_TCP:
			socktype = SOCK_STREAM;
			break;
		default:
			(void)fprintf(stderr, "unknown protocol type\n");
			return 0;
	}

	if (*sockp == RPC_ANYSOCK) {
		if ((s = socket(AF_INET, socktype, 0)) < 0) {
			perror("socket");
			return 0;
		}
		*sockp = s;
	}
	else
		s = *sockp;
	
	addr.sin_addr.s_addr = 0;
	addr.sin_family = AF_INET;
	addr.sin_port = 0;
	len = sizeof(addr);

	if (bind(s, (struct sockaddr *)&addr, len) != 0) {
		perror("bind");
		return 0;
	}
	if (getsockname(s, (struct sockaddr *)&addr, &len) < 0) {
		perror("getsockname");
		return 0;
	}
	while (!pmap_set(prognum++, vers, proto,
		ntohs(addr.sin_port))) continue;
	return (prognum-1);
}
#endif "SVR4"


/*
 * The server calls this routine when an update event occurs;  
 * It's job is to notify CM asynchronously that an
 * update has occurred.  It has to do it this
 * way (i.e. raise a signal) because the client
 * can't make an rpc call until this procedure call has
 * returned to the server.
 */
 
/* ARGSUSED */
extern Update_Status *
update_callback_1(t)
        Table_Res *t;
{
	/*	I should probably put the list of Uids in shared memory;
		It's not doing me much good to drop them on the
		floor.
	*/
	Update_Status status = update_succeeded;
	int pid = getpid();

#if 0
	cm_strcpy(callback_name, t->name);
#endif
	/*
	 * Nuke contents of all caches since the calendar has been modified
	 * We do not know which calendar this notification was for so we
	 * flush all of them.
	 *
	 * XXX dipol: This should be pushed down into the library.  This
	 *	      is the only client code which knows about the cache.
	 */
	abbcache_flush_all();

	kill(pid, SIGUSR1);
	return (&status);
}

extern void
init_agent()
{
	int s=RPC_ANYSOCK;

#ifdef SVR4
        (void)rpcb_unset(transient, AGENTVERS, NULL);
        transient = gettransient((u_long)1);

        (void) rpc_reg(transient, AGENTVERS,
                update_callback,
                (char *(*)())update_callback_1,
                xdr_Table_Res, xdr_Update_Status, "udp");
#else
        (void)pmap_unset(transient, AGENTVERS);
        transient = gettransient(IPPROTO_UDP,(u_long)1, &s);

        (void) registerrpc(transient, AGENTVERS,
                update_callback,
                (char *(*)())update_callback_1,
                xdr_Table_Res, xdr_Update_Status);
#endif "SVR4"
}

extern void
destroy_agent()
{
#ifdef SVR4
	(void) rpcb_unset(transient, AGENTVERS, NULL);
#else
	(void) pmap_unset(transient, AGENTVERS);
#endif "SVR4"
}
