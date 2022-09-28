#ifndef lint
static  char sccsid[] = "@(#)transient.c 3.1 92/04/03 Copyr 1991 Sun Microsystems, Inc.";
#endif

#include <stdio.h>
#include <rpc/rpc.h>

#ifdef SVR4
#include <netconfig.h>
#include <netdir.h>
#else
#include <sys/socket.h>
#endif "SVR4"

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
	if (!netconf) 
	    {
	    fprintf(stderr, "gettransient: getnetconfigent(%s) failed\n", "udp");
	    return 0;
	    }

	stat = netdir_getbyname(netconf, &host, &addrp);
	if (stat) 
	    {
	    fprintf(stderr, "gettransient: netdir_getbyname failed\n");
	    return 0;
	    }

 	if (addrp->n_cnt < 1) 
	    {
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
